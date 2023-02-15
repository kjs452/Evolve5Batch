/*
 * Copyright (c) 2022 Ken Stauffer
 */

/***********************************************************************
 * COMPILE/DISASSEMBLE KFORTH PROGRAMS
 *
 *
 */
#include "evolve_simulator.h"
#include "evolve_simulator_private.h"
#include <ctype.h>
#include <stdarg.h>

static void errfmt(char *errbuf, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(errbuf, 1000, fmt, ap);
	va_end(ap);
}

/*
 * Return true if 'c' is a valid character inside of a kforth word
 */
static int is_kforth_word_char(int c)
{
	if( isspace(c) || strchr(";:{}", c) || iscntrl(c) )
		return 0;
	else
		return 1;
}

/*
 * All digits (with optional leading minus sign).
 */
static int is_kforth_operand(char *word)
{
	int len;
	char *w;

	ASSERT( word != NULL );

	len = 0;
	w = word;

	if( *w == '-' ) {
		len++;
		w++;
	}

	while( *w ) {
		if( isdigit(*w) ) {
			len++;
		}
		w++;
	}

	return ( strlen(word) == len );
}

//
// derived from: djb2
//
static int hash(const char *str)
{
	unsigned long hash = 5381;
	int ch;

	while( (ch = *str++) ) {
		ch = tolower(ch);
		hash = ((hash << 5) + hash) + ch;		// hash * 33 + c
	}

    return hash % (KF_HASH_SIZE);
}

KFORTH_SYMTAB *kforth_symtab_make(KFORTH_OPERATIONS *kfops)
{
	KFORTH_SYMTAB *result;
	int hc;
	const char *name;
	int16_t *list;
	int i, j;

	ASSERT( kfops != NULL );

	result = (KFORTH_SYMTAB*) CALLOC(1, sizeof(KFORTH_SYMTAB) );
	ASSERT( result != NULL );

	for(i=0; i < KF_HASH_SIZE; i++) {
		result->hash[i].list[0] = -1;
	}

	for(i=0; i < kfops->count; i++) {
		name = kfops->table[i].name;
		hc = hash(name);

		list = result->hash[hc].list;

		j = 0;
		while( list[j] != -1 )
		{
			j += 1;
			ASSERT( j < KF_CHAIN_LEN );		// increase KF_CHAIN_LEN if this happens
		}

		list[j] = i;
		ASSERT( j+1 < KF_CHAIN_LEN );		// increate KF_CHAIN_LEN if this happens
		list[j+1] = -1;
	}

	return result;
}

void kforth_symtab_delete(KFORTH_SYMTAB *kfst)
{
	ASSERT( kfst != NULL );
	FREE( kfst );
}

static int lookup_opcode_sym(KFORTH_SYMTAB *kfst, KFORTH_OPERATIONS *kfops, char *word)
{
	int16_t* list;
	const char *name;
	int hc;
	int opcode;
	int i;

	hc = hash(word);
	list = kfst->hash[hc].list;
	i = 0;
	while( list[i] != -1 )
	{
		opcode = list[i];
		name = kfops->table[ opcode ].name;

		if( stricmp(name, word) == 0 )
		{
			return opcode;
		}

		i += 1;
	}

	return -1;
}

#ifndef KFORTH_COMPILE_FAST
/*
 * Return the opcode that corresponds to 'word' (or return -1).
 */
static int lookup_opcode(KFORTH_OPERATIONS *kfops, char *word)
{
	int i;

	ASSERT( kfops != NULL );
	ASSERT( word != NULL );

	for(i=0; i<kfops->count; i++) {
		if( stricmp(kfops->table[i].name, word) == 0 ) {
			return i;
		}
	}

	return -1;
}
#endif

struct kforth_label_usage {
	int lineno;
	int cb;
	int pc;
	struct kforth_label_usage *next;
};

struct kforth_label {
	char	*name;
	int	lineno;
	int	cb;

	struct 	kforth_label_usage *usage;
	struct kforth_label *next;
};

static struct kforth_label *LabelList;

static struct kforth_label *create_label(char *word)
{
	struct kforth_label *label, *lp;

	ASSERT( word != NULL );

	label = (struct kforth_label*) CALLOC(1, sizeof(struct kforth_label));
	ASSERT( label != NULL );
	label->name = STRDUP(word);

	if( LabelList == NULL ) {
		LabelList = label;
	} else {
		for(lp=LabelList; lp->next; lp=lp->next)
			;
		lp->next = label;
	}

	return label;
}

static struct kforth_label *lookup_label(char *word)
{
	struct kforth_label *label;

	ASSERT( word != NULL );

	for(label=LabelList; label; label=label->next) {
		if( stricmp(label->name, word) == 0 )
			break;
	}

	return label;
}

static void add_label_usage(struct kforth_label *label, int lineno, int cb, int pc)
{
	struct kforth_label_usage *usage, *up;

	ASSERT( label != NULL );

	usage = (struct kforth_label_usage*) CALLOC(1, sizeof(struct kforth_label_usage));
	ASSERT( usage != NULL );

	usage->lineno = lineno;
	usage->cb = cb;
	usage->pc = pc;

	if( label->usage == NULL ) {
		label->usage = usage;
	} else {
		for(up=label->usage; up->next; up=up->next)
			;

		up->next = usage;
	}
}

static void delete_labels(void)
{
	struct kforth_label *label, *label_next;
	struct kforth_label_usage *usage, *usage_next;

	for(label=LabelList; label; label=label_next) {
		for(usage=label->usage; usage; usage=usage_next) {
			usage_next = usage->next;
			FREE(usage);
		}
		label_next = label->next;
		FREE(label->name);
		FREE(label);
	}

	LabelList = NULL;
}

typedef enum {
	NUMBER,
	OPCODE,
	ENDCB,
} OPCODE_TYPE;

/*
 * Compile a number of instruction into (cb,pc).
 * If (cb,pc) is not in bounds (exceeds 16383) b0011 1111 1111 1111
 * then out_of_bounds flag will be set and errbuf contains an error.
 *
 * If this flag is already set, then compile_opcode() is a no-op.
 *
 * opcode_type=ENDCB is called when '} for end of code block. make sure the empty code blocks are created.
 *
 */
static void compile_opcode(KFORTH_PROGRAM *kfp, int cb, int pc, OPCODE_TYPE opcode_type, KFORTH_INTEGER value,
									int *out_of_bounds, char *errbuf)
{
	int i, len;

	ASSERT( kfp != NULL );
	ASSERT( cb >= 0 );
	ASSERT( pc >= 0 );
	ASSERT( opcode_type == OPCODE || opcode_type == NUMBER || opcode_type == ENDCB );

	if( *out_of_bounds != 0 ) {
		return;
	}

	if( cb >= 16383 )
	{
		*out_of_bounds = 1;
		errfmt(errbuf, "too many code blocks, exceeds 16383.");
		return;
	}

	if( pc >= 16383 )
	{
		*out_of_bounds = 1;
		errfmt(errbuf, "code block %d is too big, exceeds 16383 instructions.", cb);
		return;
	}

	/*
	 * Grow program to fit code block 'cb'.
	 */
	if( cb >= kfp->nblocks ) {
		kfp->block = (KFORTH_INTEGER**)
				REALLOC(kfp->block, sizeof(KFORTH_INTEGER*) * (cb+1) );

		for(i=kfp->nblocks; i<cb+1; i++) {
			kfp->block[i] = ((KFORTH_INTEGER*) CALLOC(1, sizeof(KFORTH_INTEGER))) + 1;
			kfp->block[i][-1] = 0;
		}
		kfp->nblocks = cb+1;
	}

	if( opcode_type == ENDCB )
	{
		// nothing to do
	}
	else
	{
		/*
		 * Grow code block (include room for the length prefix)
		 */
		len = kforth_program_cblen(kfp, cb);
		if( pc >= len ) {
			kfp->block[cb] = ((KFORTH_INTEGER*) REALLOC(kfp->block[cb]-1, sizeof(KFORTH_INTEGER) * (pc+2) )) + 1;
			kfp->block[cb][-1] = pc+1;
		}

		/*
		 * Set opcode/operand at (cb, pc)
		 */
		if( opcode_type == NUMBER ) {
			value = value | 0x8000;
		} else {
			ASSERT( value >= 0 && value < KFORTH_OPS_LEN );
			value = value;
		}
		kfp->block[cb][pc] = value;
	}

}

/***********************************************************************
 * Compile the kforth program in the string: 'program_text' 
 */
KFORTH_PROGRAM *kforth_compile_kfst(const char *program_text, KFORTH_SYMTAB *kfst, KFORTH_OPERATIONS *kfops, char *errbuf)
{
	KFORTH_PROGRAM *kfp;
	int lineno, in_comment, error;
	int save_cb, cb, pc, sp, opcode;
	KFORTH_INTEGER operand;
	int64_t value;
	const char *p;
	char word[1000], *w;
	int cb_stack[100], pc_stack[100];
	struct kforth_label *label;
	struct kforth_label_usage *usage;
	int next_code_block;
	int oob;			/* out of bounds flag */
	char bbuf[500];		/* out of bounds error buffer */

	ASSERT( program_text != NULL );
	ASSERT( kfst != NULL );
	ASSERT( kfops != NULL );
	ASSERT( errbuf != NULL );

	kfp = (KFORTH_PROGRAM*) CALLOC(1, sizeof(KFORTH_PROGRAM));
	ASSERT( kfp != NULL );

	kfp->nblocks = 1;
	kfp->block = (KFORTH_INTEGER**) CALLOC(1, sizeof(KFORTH_INTEGER*));

	kfp->block[0] = ((KFORTH_INTEGER*) CALLOC(1, sizeof(KFORTH_INTEGER))) + 1;
	kfp->block[0][-1] = 0;

	next_code_block = 0;
	sp = 0;
	cb = -1;
	pc = 0;
	lineno = 1;
	in_comment = 0;
	error = 0;
	p = program_text;
	oob = 0;
	while( *p != '\0' && !error ) {
		if( isspace(*p) || in_comment ) {
			if( *p == '\n' ) {
				in_comment = 0;
				lineno++;
			}
			p++;

		} else if( *p == ';' ) {
			in_comment = 1;
			p++;

		} else if( *p == '{' ) {
			cb_stack[sp] = cb;
			pc_stack[sp] = pc;
			sp++;

			cb = next_code_block++;
			pc = 0;

			p++;

		} else if( *p == '}' ) {
			compile_opcode(kfp, cb, pc, ENDCB, 0, &oob, bbuf);
			sp--;
			if( sp < 0 ) {
				errfmt(errbuf, "Line: %d, too many close braces", lineno);
				error = 1;
			} else if( sp > 0 ) {
				save_cb = cb;
				cb = cb_stack[sp];
				pc = pc_stack[sp];

				/* compile absolute code block number */
				compile_opcode(kfp, cb, pc, NUMBER, save_cb, &oob, bbuf);
				pc++;
			}
			p++;

		} else if( iscntrl(*p) ) {
			errfmt(errbuf, "Line: %d, invalid char %d", lineno, *p);
			error = 1;

		} else {
			w = word;
			while( is_kforth_word_char(*p) ) {
				*w++ = *p++;
			}
			*w = '\0';

			if( *p == ':' ) {
#ifndef KFORTH_COMPILE_FAST
				opcode = lookup_opcode(kfops, word);
#else
				opcode = lookup_opcode_sym(kfst, kfops, word);
#endif
				if( opcode >= 0 ) {
					errfmt(errbuf, "Line: %d, label '%s' clashes with instruction",
						lineno, word);
					error = 1;

				} else if( is_kforth_operand(word) ) {
					errfmt(errbuf, "Line: %d, numbers cannot be a label '%s'",
						lineno, word);
					error = 1;

				} else {
					p++;
					label = lookup_label(word);
					if( label == NULL ) {
						label = create_label(word);
						label->cb = next_code_block;
						label->lineno = lineno;
					} else {
						if( label->lineno != -1 ) {
							/* error: mutliply define labels */
							errfmt(errbuf,
								"Line: %d, symbol '%s' multiply defined",
								lineno, word);
							error = 1;
						} else {
							label->lineno = lineno;
							label->cb = next_code_block;
						}
					}
				}

			} else if( sp == 0 ) {
				errfmt(errbuf, "Line: %d, '%s' appears outside of a code block",
						lineno, word);
				error = 1;

			} else {
#ifndef KFORTH_COMPILE_FAST
				opcode = lookup_opcode(kfops, word);
#else
				opcode = lookup_opcode_sym(kfst, kfops, word);
#endif
				if( opcode >= 0 ) {
					compile_opcode(kfp, cb, pc, OPCODE, opcode, &oob, bbuf);

				} else if( is_kforth_operand(word) ) {
					value = atoll(word);
					if( value > 16383 ) {
							errfmt(errbuf,
								"Line: %d, operand '%s' too big. maximum literal value is 16383",
								lineno, word);
							error = 1;
					} else if( value < -16384 ) {
							errfmt(errbuf,
								"Line: %d, operand '%s' too small. minimum literal value is -16384",
								lineno, word);
							error = 1;
					}
					operand = value;
					compile_opcode(kfp, cb, pc, NUMBER, operand, &oob, bbuf);

				} else {
					label = lookup_label(word);
					if( label == NULL ) {
						label = create_label(word);
						label->lineno = -1;
						label->cb = 0;
						add_label_usage(label, lineno, cb, pc);
						compile_opcode(kfp, cb, pc, NUMBER, 0, &oob, bbuf);
					} else {
						if( label->lineno == -1 ) {
							add_label_usage(label, lineno, cb, pc);
						} else {
							/* compile absolute code block number */
							compile_opcode(kfp, cb, pc, NUMBER, label->cb, &oob, bbuf);
						}
					}
				}
				pc++;
			}
		}
	}

	if( error ) {
		delete_labels();
		kforth_delete(kfp);
		return NULL;
	}

	if( sp > 0 ) {
		delete_labels();
		kforth_delete(kfp);
		errfmt(errbuf, "Line: %d, missing close braces", lineno);
		return NULL;
	}

	if( oob != 0 ) {
		delete_labels();
		kforth_delete(kfp);
		errfmt(errbuf, "Line: %d, %s", lineno, bbuf);
		return NULL;
	}

	/*
	 * Make sure all labels are defined
	 */
	for(label=LabelList; label; label=label->next) {
		if( label->lineno == -1 ) {
			lineno = label->usage->lineno;
			errfmt(errbuf, "Line: %d, undefined label '%s'", lineno, label->name);
			kforth_delete(kfp);
			delete_labels();
			return NULL;
		}

		/*
		 * back patch all usage instances of label
		 */
		for(usage=label->usage; usage; usage=usage->next) {
			cb = usage->cb;
			pc = usage->pc;
			kfp->block[cb][pc] = (0x8000 | label->cb);
		}
	}

	delete_labels();

	return kfp;
}

KFORTH_PROGRAM *kforth_compile(const char *program_text, KFORTH_OPERATIONS *kfops, char *errbuf)
{
	KFORTH_PROGRAM *kfp;
	KFORTH_SYMTAB *kfst;

	kfst = kforth_symtab_make(kfops);

	kfp = kforth_compile_kfst(program_text, kfst, kfops, errbuf);

	kforth_symtab_delete(kfst);

	return kfp;
}

void kforth_program_init(KFORTH_PROGRAM *kfp)
{
	memset(kfp, 0, sizeof(KFORTH_PROGRAM));
}

/*
 * De-allocate sub-objects inside of a 'kfp' but don't delete kfp.
 * Remember, this is the structure being used:
 *
 *    kfp->block[cb] ----+
 *                       |
 *                       v
 *              +-----+-----+-----+-----+-----+-----+-----+
 *              |  6  | if  | 2   |  3  | nop |call | dup |
 *              +-----+-----+-----+-----+-----+-----+-----+
 *                -1     0     1     2     3     4     5
 */
void kforth_program_deinit(KFORTH_PROGRAM *kfp)
{
	int cb;

	for(cb=0; cb < kfp->nblocks; cb++ ) {
		FREE( kfp->block[cb]-1 );
	}

	FREE( kfp->block );
}

/***********************************************************************
 * Delete the KFORTH program 'kfp'
 */
void kforth_delete(KFORTH_PROGRAM *kfp)
{
	ASSERT( kfp != NULL );

	kforth_program_deinit(kfp);

	FREE( kfp );
}

static void append_to_string(char **str, int *size, int *len, char *buf)
{
	int buflen, new_size;

	ASSERT( str != NULL );
	ASSERT( *str != NULL );
	ASSERT( *size >= 0 );
	ASSERT( *len >= 0 );
	ASSERT( buf != NULL );

	buflen = (int) strlen(buf);

	if( (size-len) <= buflen ) {
		new_size = *size + 1000;
		*str = (char*) REALLOC(*str, new_size);
		*size = new_size;
	}

	strcat(*str, buf);
	*len += buflen;

}

static void disassembly_fmt(char *buf, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, 1000, fmt, ap);
	va_end(ap);
}

/***********************************************************************
 * Convert the program 'kfp' into a readable string.
 * And also create a table (KFORTH_DISASSEMBLY_POS) that maps
 * character offsets in the string to 'cb' and 'pc'.
 *
 * 'width' is the maximum column width to break code blocks at.
 * A value of '80', for example would break lines whenever the next
 * intruction to be added would exceed this width.
 *
 * If 'want_cr' is non-zero then we terminate lines with \r\n instead of
 * just \n. (this is needed when populating windows CEdit controls!!!).
 *
 * This is also needed by MacOS NNSTextView, the breakpoint symbol
 * will be positioned incorrectly if want_cr isn't 1.
 * (ViewOrganism screen, KforthInterpreter screen)
 *
 * HOW TO USE POS TABLE:
 *	The 'pos' table allows the caller to map a (cb, pc) to text contained
 *	int program_text. Simply scan the table (from 0 .. pos_len) looking for
 *	a matching (cb, pc) pair. When such an entry is found, you have
 *	character offsets for that item.
 *
 *	When pc equals "code block length" then
 *	this entry stores the offset to the closing curly brace.
 *
 *	When pc is -1 then this entry points to the label for the code block.
 *
 *
 */
KFORTH_DISASSEMBLY *kforth_disassembly_make(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, int width, int want_cr)
{
	char buf[1000];
	char buf2[1000];
	const char *nlstr;
	int cb, pc, opcode;
	int max_opcode;
	int line_length, i, len, block_len;
	KFORTH_DISASSEMBLY *result;
	KFORTH_INTEGER value;

	char *program;
	int plen, psize;

	ASSERT( kfp != NULL );
	ASSERT( width >= 20 );

	if( want_cr ) {
		nlstr = "\r\n";
	} else {
		nlstr = "\n";
	}

	result = (KFORTH_DISASSEMBLY*) CALLOC(1, sizeof(KFORTH_DISASSEMBLY));
	ASSERT( result != NULL );

	result->program_text = NULL;

	/*
	 * Compute length of array
	 */
	result->pos_len = kfp->nblocks;
	for(cb=0; cb < kfp->nblocks; cb++) {
		result->pos_len += kforth_program_cblen(kfp, cb) + 1;
	}

	result->pos = (KFORTH_DISASSEMBLY_POS*) CALLOC(result->pos_len, sizeof(KFORTH_DISASSEMBLY_POS));
	ASSERT( result->pos != NULL );

	max_opcode = kforth_ops_max_opcode(kfops);

	psize = 1000;
	program = (char*) CALLOC(psize, sizeof(char));
	plen = 0;
	program[0] = '\0';
	i = 0;
	for(cb=0; cb < kfp->nblocks; cb++) {
		if( cb == 0 ) {
			disassembly_fmt(buf, "main");
		} else {
			disassembly_fmt(buf, "row%d", cb);
		}

		len = (int) strlen(buf);

		result->pos[i].cb	= cb;
		result->pos[i].pc	= -1;
		result->pos[i].start_pos= plen;
		result->pos[i].end_pos	= plen + len - 1;
		i++;

		disassembly_fmt(buf2, "%s:%s{%s    ", buf, nlstr, nlstr);
		append_to_string(&program, &psize, &plen, buf2);

		line_length = 4;
		block_len = kforth_program_cblen(kfp, cb);
		for(pc=0; pc < block_len; pc++) {

			if( line_length >= width ) {

				disassembly_fmt(buf, "%s    ", nlstr);
				line_length = 4;

				append_to_string(&program, &psize, &plen, buf);
			}

			strcpy(buf, "  ");
			line_length += 2;
			append_to_string(&program, &psize, &plen, buf);

			opcode = kfp->block[cb][pc];
			if( opcode & 0x8000 ) {
				value = opcode & 0x7fff;
				if( value & 0x4000 )       					// sign extention
					value |= 0x8000;
				disassembly_fmt(buf, "%d", value);

			} else {
				if( opcode >= 0 && opcode <= max_opcode ) {
					disassembly_fmt(buf, "%s", kfops->table[opcode].name);
				} else {
					ASSERT(0);
				}
			}

			len = (int) strlen(buf);
			line_length += len;

			result->pos[i].cb	= cb;
			result->pos[i].pc	= pc;
			result->pos[i].start_pos= plen;
			result->pos[i].end_pos	= plen + len - 1;
			i++;

			append_to_string(&program, &psize, &plen, buf);
		}

		/*
		 * We are at the end of a code block, add closing '}' and add entry.
		 */
		disassembly_fmt(buf, " %s", nlstr);
		append_to_string(&program, &psize, &plen, buf);

		disassembly_fmt(buf, "}");
		len = (int) strlen(buf);

		result->pos[i].cb	= cb;
		result->pos[i].pc	= pc;
		result->pos[i].start_pos= plen;
		result->pos[i].end_pos	= plen + len - 1;
		i++;

		append_to_string(&program, &psize, &plen, buf);

		disassembly_fmt(buf, "%s%s", nlstr, nlstr);
		append_to_string(&program, &psize, &plen, buf);
	}

	ASSERT( i == result->pos_len );

	result->program_text = program;

	return result;
}

/***********************************************************************
 * Free a KFORTH_DISASSEMBLY object that was created from
 * 'kforth_disassembly_make()'
 *
 *
 */
void kforth_disassembly_delete(KFORTH_DISASSEMBLY *kfd)
{
	ASSERT( kfd != NULL );

	FREE(kfd->program_text);
	FREE(kfd->pos);
	FREE(kfd);
}


/*
 * return total number of instructions and numbers
 * that comprise the program 'kfp'.
 */
int kforth_program_length(KFORTH_PROGRAM *kfp)
{
	int cb, len;

	ASSERT( kfp != NULL );

	len = 0;
	for(cb=0; cb < kfp->nblocks; cb++) {
		len += kforth_program_cblen(kfp, cb);
	}

	return len;
}

/*
 * Return memory size of 'kfp' in bytes.
 */
int kforth_program_size(KFORTH_PROGRAM *kfp)
{
	int cb, len, size;

	ASSERT( kfp != NULL );

	size = 0;

	size += sizeof(KFORTH_PROGRAM);
	size += kfp->nblocks * sizeof(KFORTH_INTEGER*);
	for(cb=0; cb < kfp->nblocks; cb++) {
		len = kforth_program_cblen(kfp, cb);
		size += len * sizeof(KFORTH_INTEGER);
	}

	return size;
}

/*
 * Perform a simple parse of program and skip comments
 * and count braces. Then find 'symbol' and return it label number.
 * Assumes symbol and ':' are touching.
 *
 *	"label:"		not "label : "
 *
 */
int kforth_program_find_symbol(const char *program, const char *symbol)
{
	char buf[ 1000 ];
	int cb;
	int len;
	int in_comment;
	const char *p;

	strcpy(buf, symbol);
	strcat(buf, ":");
	len = (int)strlen(buf);

	cb = 0;
	in_comment = 0;
	for(p=program; *p; p++)
	{
		if( *p == ';' ) { in_comment = 1; }
		if( *p == '\n' ) { in_comment = 0; }

		if( !in_comment && strncmp(buf, p, len) == 0 ) {
			return cb;
		}

		if( !in_comment && *p == '{' ) {
			cb += 1;
		}
	}

	return -1;
}

#if 0
static void debug_dump_kfops(KFORTH_OPERATIONS* kfops1, KFORTH_OPERATIONS* kfops2, KFORTH_PROGRAM *kfp, int *map)
{
	int i, j1, j2, op;
	int key1, key2;
	int cb, pc;
	int fail, different, MaxOps;
	const char *instr1, *instr2;

	printf("kfops1->count = %d, kfops2->count = %d\n", kfops1->count, kfops2->count);

	MaxOps = (kfops1->count > kfops2->count) ? kfops1->count : kfops2->count;

	// iterate down both instruction tables
	different = 0;
	j1 = 0;
	j2 = 0;
	for(i=0; i < MaxOps; i++) {

		if( j1 >= kfops1->count )
			key1 = 9999;
		else
			key1 = kfops1->table[j1].key;

		if( j2 >= kfops2->count )
			key2 = 9999;
		else
			key2 = kfops2->table[j2].key;

		if( key1 == key2 && key1 != 9999 ) {
			instr1 = kfops1->table[j1].name;
			instr2 = kfops2->table[j2].name;
			j1++;
			j2++;
		} else if( key1 < key2 ) {
			different = 1;
			instr1 = "<null1>";
			instr2 = kfops2->table[j2].name;
			j1++;
		} else if( key1 > key2 ) {
			different = 1;
			instr1 = kfops1->table[j1].name;
			instr2 = "<null2>";
			j2++;
		}

		printf("i=%3d : %-20s %-20s    %-20s  %-20s  j1=%d, j2=%d  map[%d] = %d\n",
					i,
					kfops1->table[i].name,
					kfops2->table[i].name,
					instr1,
					instr2,
					j1,
					j2,
					i,
					map[i] );
	}

	printf("different = %d\n", different);

	if( ! different )
	{
		return;
	}

	fail = 0;
	for(cb=0; cb < kfp->nblocks; cb++)
	{
		for(pc=0; kfp->block[cb][pc] != 0; pc++) {
			op = kfp->block[cb][pc];

			if( (op & 0x8000) == 0 ) {
				if( map[op] != -1 ) {
					// kfp->block[cb][pc] = map[op];
					printf("(%d,%d) -> map[%d] = %d\n", cb, pc, op, map[op]);
				} else {
					fail = 1;
					break;
				}
			}
		}
	}

	printf("fail = %d\n", fail);
}
#endif

// NEW IMPLEMENTATION
//
// re-map instructions.
//
// Modify 'kfp' such that it is compatible from 'kfops1'.
// 'kfp' is assumed to currently compatible with 'kfops2'
//
// Assumes kfops1 and kfops2 are both derived from the same master list, with
// only some items removed. Ordering of both lists is assumed to be the same.
//
// This fails if 'kfops1' lacks an instruction required by 'kfp'.
//
// Returns the number of 1-to-1 mappings that were written to 'map'
//
static int remap_instructions_join(
					KFORTH_OPERATIONS* kfops1,
					KFORTH_OPERATIONS* kfops2,
					int *map,
					int start1,
					int end1,
					int start2,
					int end2 )
{
	int j1, j2;
	int key1, key2;
	int count;

	// iterate down both instruction tables
	count = 0;
	j1 = start1;
	j2 = start2;

	while( j1 <= end1 || j2 <= end2 ) {
		if( j1 > end1 )
			key1 = 9999;
		else
			key1 = kfops1->table[j1].key;

		if( j2 > end2 )
			key2 = 9999;
		else
			key2 = kfops2->table[j2].key;

		if( key1 == key2 && key1 != 9999 ) {
			map[j2] = j1;
			if( j1 == j2 )
				count += 1;
			j1++;
			j2++;
		} else if( key1 < key2 ) {
			j1++;
		} else if( key1 > key2 ) {
			j2++;
		}
	}

	return count;
}

/*
 *
 *	kfops1 ->						kfops2 ->
 *
 *		+---------------+				+---------------+
 *		|				|				|				|
 *		| Protected		|				|				|
 *		| Instructions	|				| Protected		|
 *		|		P1		|				| Instructions	|
 *		|				|				|		P2		|
 *		+---------------+				|				|
 *		|				|				|				|
 *		|				|				|				|
 *		|				|				|				|
 *		|				|				+---------------+
 *		|				|				|				|
 *		| Unprotected	|				|				|
 *		| Instructions	|				| Unprotected	|
 *		|	U1			|				| Instructions	|
 *		|				|				|		U2		|
 *		|				|				|				|
 *		|				|				|				|
 *		|				|				|				|
 *		|				|				+---------------+
 *		|				|
 *		|				|
 *		|				|
 *		+---------------+
 *
 *	Must perform different joins:
 *		P1 x P2
 *		P1 x U2
 *		U1 x P2
 *		U1 x U2
 *
 * All lists are sorted by 'key'.
 *
 * Each time the map[] is updated, hopefully all opcodes in kfp have a home in the map.
 *
 *
 * re-map instructions.
 *
 * Modify 'kfp' such that it is compatible from 'kfops1'.
 * 'kfp' is assumed to currently compatible with 'kfops2'
 *
 * Assumes kfops1 and kfops2 are both derived from the same master list, with
 * only some items removed. Ordering of both lists is assumed to be the same.
 *
 * This fails if 'kfops1' lacks an instruction required by 'kfp'.
 *
 * RETURNS:
 * Returns the number of instructions that mapped directly between sets.
 * 'map' is populated is a number remapping, or -1 of no mapping exists.
 *
 */
static int generate_instruction_map(KFORTH_OPERATIONS* kfops1, KFORTH_OPERATIONS* kfops2, int map[KFORTH_OPS_LEN])
{
	int p1_start, p1_end;
	int p2_start, p2_end;
	int u1_start, u1_end;
	int u2_start, u2_end;
	int i, c1, c2, c3, c4;
	int max;

	ASSERT( kfops1 != NULL );
	ASSERT( kfops2 != NULL );
	ASSERT( kfops1->count <= KFORTH_OPS_LEN );
	ASSERT( kfops2->count <= KFORTH_OPS_LEN );

	p1_start = 0;
	p1_end = kfops1->nprotected-1;

	p2_start = 0;
	p2_end = kfops2->nprotected-1;

	u1_start = kfops1->nprotected;
	u1_end = kfops1->count-1;

	u2_start = kfops2->nprotected;
	u2_end = kfops2->count-1;

	max = (kfops1->count > kfops2->count) ? kfops1->count : kfops2->count;

	for(i=0; i < max; i++) {
		map[i] = -1;
	}

	c1 = remap_instructions_join(kfops1, kfops2, map, p1_start, p1_end, p2_start, p2_end);
	c2 = remap_instructions_join(kfops1, kfops2, map, p1_start, p1_end, u2_start, u2_end);
	c3 = remap_instructions_join(kfops1, kfops2, map, u1_start, u1_end, p2_start, p2_end);
	c4 = remap_instructions_join(kfops1, kfops2, map, u1_start, u1_end, u2_start, u2_end);

	return c1 + c2 + c3 + c4;
}

/*
 * re-map instructions.
 *
 * Modify 'kfp' such that it is compatible from 'kfops1'.
 * 'kfp' is assumed to currently compatible with 'kfops2'
 * Returns 1 on success, 0 on fail
 */
int kforth_remap_instructions(KFORTH_OPERATIONS* kfops1, KFORTH_OPERATIONS* kfops2, KFORTH_PROGRAM *kfp)
{
	int fail;
	int cb, pc, op, len;
	int map[ KFORTH_OPS_LEN ];

	ASSERT( kfops1 != NULL );
	ASSERT( kfops2 != NULL );
	ASSERT( kfp != NULL );
	ASSERT( kfops1->count <= KFORTH_OPS_LEN );
	ASSERT( kfops2->count <= KFORTH_OPS_LEN );

	len = generate_instruction_map(kfops1, kfops2, map);
	if( len == kfops2->count ) {
		//
		// every kfops2 instruction was found to map to the same opcode in kfops1
		// we can return success without needing to remap any instructions.
		//
		return 1;
	}

	fail = 0;
	for(cb=0; cb < kfp->nblocks; cb++)
	{
		len = kforth_program_cblen(kfp, cb);
		for(pc=0; pc < len; pc++) {
			op = kfp->block[cb][pc];

			if( (op & 0x8000) == 0 ) {
				if( map[op] != -1 ) {
					kfp->block[cb][pc] = map[op];
				} else {
					fail = 1;
					break;
				}
			}
		}
	}

	return ! fail;
}

/*
 * re-map instructions in a code block.
 *
 * Modify 'block' such that it is compatible from 'kfops1'.
 * 'block' is assumed to currently compatible with 'kfops2'.
 * Returns 1 on success, 0 on fail
 */
int kforth_remap_instructions_cb(KFORTH_OPERATIONS* kfops1, KFORTH_OPERATIONS* kfops2, KFORTH_INTEGER *block)
{
	int fail;
	int pc, op, len, num;
	int map[ KFORTH_OPS_LEN ];

	ASSERT( kfops1 != NULL );
	ASSERT( kfops2 != NULL );
	ASSERT( kfops1->count <= KFORTH_OPS_LEN );
	ASSERT( kfops2->count <= KFORTH_OPS_LEN );
	ASSERT( block != NULL );

	num = generate_instruction_map(kfops1, kfops2, map);
	if( num == kfops2->count ) {
		//
		// every kfops2 instruction was found to map to the same opcode in kfops1
		// we can return success without needing to remap any instructions.
		//
		return 1;
	}

	len = block[-1];

	fail = 0;
	for(pc=0; pc < len; pc++) {
		op = block[pc];

		if( (op & 0x8000) == 0 ) {
			if( map[op] != -1 ) {
				block[pc] = map[op];
			} else {
				fail = 1;
				break;
			}
		}
	}

	return ! fail;
}

/*
 * copy 'thing' string to *dst.
 * set dst to point to the null after copied string.
 */
static void comment_add(char **dst, const char *thing)
{
	char *r;
	const char *q;

	r = *dst;
	q = thing;
	while( *q != '\0' ) {
		*r++ = *q++;
	}
	*r = '\0';

	*dst = r;
}

/*
 * add mode if non-zero. check width, add newline if width reached
 */
static void comment_add_prop(int value, const char *property, char **dst, int *width)
{
	int MAX_WIDTH = 55;
	char buf[ 1000 ];

	if( value != 0 ) {
		if( *width == 0 ) {
			comment_add(dst, ";    ");
			*width += 5;
		}

		snprintf(buf, sizeof(buf), "%s=%d ", property, value);
		comment_add(dst, buf);

		*width += strlen(buf);
		if( *width >= MAX_WIDTH ) {
			comment_add(dst, "\n");
			*width = 0;
		}
	}
}

/*
 * add mode if non-zero. check width, add newline if width reached
 */
static void comment_add_instr(const char *name, char **dst, int *width)
{
	int MAX_WIDTH = 55;
	char buf[ 1000 ];

	if( *width == 0 ) {
		comment_add(dst, ";    ");
		*width += 5;
	}

	snprintf(buf, sizeof(buf), "%s ", name);
	comment_add(dst, buf);

	*width += strlen(buf);
	if( *width >= MAX_WIDTH ) {
		comment_add(dst, "\n");
		*width = 0;
	}
}

/*
 * Construct a string that is a KFORTH comment.
 * This string encodes all the protections, modes and mutation
 * settings. Anything that is 0 is omitted.
 *
 * width = 55
*******************************************************
; Strain 7: nibbler
; Protected Code Blocks: 2
; Protected Instructions: MAKE-SPORE SPAWN FOOP YOU dup
;    crap R0 R1 R9 TRAP9 TRAP7 TRAP6 EAT SEND-ENERGY
; Instruction Modes: EM=1664 GS=20 GE=100 SE=0 MOM=0
;     MBM=0 SEM=0 KPM=0 SM=0 LM=0 SHM=0 CMM=0 CSM=0
;     OMM=0 EXM=0 RDM=0 WRM=0 SAM=0 RM=0
; MaxApply=10 MaxCB=30 MergeMode=0 StrandLen=4
;
 *
 */
char *kforth_metadata_comment_make(int strain, STRAIN_OPTIONS *strop, KFORTH_MUTATE_OPTIONS *kfmo, KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp)
{
	char result[ 10*1000 ];
	char buf[ 1000 ];
	char *p;
	int width, i;

	ASSERT( strain >= 0 && strain <= 7 );
	ASSERT( strop != NULL );
	ASSERT( kfmo != NULL );
	ASSERT( kfops != NULL );
	ASSERT( kfp != NULL );

	p = result;

	snprintf(buf, sizeof(buf), "; Strain %d: %s\n", strain, strop->name);
	comment_add(&p, buf);

	snprintf(buf, sizeof(buf), "; Protected Code Blocks: %d\n", kfp->nprotected);
	comment_add(&p, buf);

	snprintf(buf, sizeof(buf), "; Protected Instructions: ");
	comment_add(&p, buf);

	width = strlen("; Protected Instructions: ");

	for(i=0; i < kfops->nprotected; i++) {
		comment_add_instr(kfops->table[i].name, &p, &width);
	}

	if( width != 0 ) {
		comment_add(&p, "\n");
		width = 0;
	}

	snprintf(buf, sizeof(buf), "; Instruction Modes: ");
	comment_add(&p, buf);

	width = strlen("; Instruction Modes: ");

	comment_add_prop(strop->look_mode, "LM", &p, &width);
	comment_add_prop(strop->eat_mode, "EAM", &p, &width);
	comment_add_prop(strop->make_spore_mode, "MSM", &p, &width);
	comment_add_prop(strop->make_spore_energy, "MSE", &p, &width);
	comment_add_prop(strop->cmove_mode, "CMM", &p, &width);
	comment_add_prop(strop->omove_mode, "OMM", &p, &width);
	comment_add_prop(strop->grow_mode, "GM", &p, &width);
	comment_add_prop(strop->grow_energy, "GE", &p, &width);
	comment_add_prop(strop->grow_size, "GS", &p, &width);
	comment_add_prop(strop->rotate_mode, "ROM", &p, &width);
	comment_add_prop(strop->cshift_mode, "CSM", &p, &width);
	comment_add_prop(strop->make_organic_mode, "MOM", &p, &width);
	comment_add_prop(strop->make_barrier_mode, "MBM", &p, &width);
	comment_add_prop(strop->exude_mode, "EXM", &p, &width);
	comment_add_prop(strop->shout_mode, "SHM", &p, &width);
	comment_add_prop(strop->spawn_mode, "SPM", &p, &width);
	comment_add_prop(strop->listen_mode, "LIM", &p, &width);
	comment_add_prop(strop->broadcast_mode, "BM", &p, &width);
	comment_add_prop(strop->say_mode, "SAM", &p, &width);
	comment_add_prop(strop->send_energy_mode, "SEM", &p, &width);
	comment_add_prop(strop->read_mode, "RDM", &p, &width);
	comment_add_prop(strop->write_mode, "WRM", &p, &width);
	comment_add_prop(strop->key_press_mode, "KPM", &p, &width);
	comment_add_prop(strop->send_mode, "SNDM", &p, &width);

	if( width != 0 ) {
		comment_add(&p, "\n");
		width = 0;
	}

	comment_add(&p, "; ");
	width = 2;
	comment_add_prop(kfmo->max_apply, "MaxApply", &p, &width);
	comment_add_prop(kfmo->max_code_blocks, "MaxCB", &p, &width);
	comment_add_prop(kfmo->merge_mode, "MergeMode", &p, &width);
	comment_add_prop(kfmo->xlen, "StrandLen", &p, &width);

	if( width != 0 ) {
		comment_add(&p, "\n");
		width = 0;
	}

	return STRDUP(result);
}

void kforth_metadata_comment_delete(char *str)
{
	ASSERT( str != NULL );
	FREE(str);
}
