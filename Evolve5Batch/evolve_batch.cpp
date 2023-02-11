/***********************************************************************
 * Evolve Batch Simulator
 *
 * This command has several distinct modes of operation. This is controlled
 * by the first argument on the command line. Modes cannot be combined.
 *
 * MODE FLAG	DESCRIPTION
 * 's'		Simulate mode
 * 'sf'		Simulate forever mode
 * 't'		generate terrain from image
 *
 * 'p'		Print information about simulation files
 *
 * 'k'		KFORTH Interpreter Mode
 *
 * '='		Compare two sim files (used for debugging)
 *
 * 'rc'		Pick Random Creature (output in ASCII format)
 *
 * '1s'		Simulate until 1 strain left. print strain that wins and
 *		in how many steps. Also write results to another file.
 *
 * --------------------------------------------------------------------------------------
 * SIMULATING:
 *	evolve_batch s 24h infile.evolve outfile.evolve		<- hours
 *	evolve_batch s 60m infile.evolve outfile.evolve		<- minute
 *	evolve_batch s 3600s infile.evolve outfile.evolve	<- seconds
 *	evolve_batch s 1000u infile.evolve outfile.evolve	<- steps
 *
 * It is okay for infile and outfile to be the same filename.
 *
 * --------------------------------------------------------------------------------------
 * TERRAIN
 *	I used evolve_batch to house the interface to image2terrain(). It reads
 * an image via stb_image and produces a evolve terrain file (a subset of the normal simulation file)
 *
 *	evolve_batch t infile.gif min max outfile.txt			<--- min and max are color values for grey scale
 *	evolve_batch t bob.png 128 255 outfile.txt			<--- min and max are color values for grey scale
 *
 * --------------------------------------------------------------------------------------
 * PRINT INFORMATION:
 *	evolve_batch p foobar.evolve		<- will print info. about foobar.evolve
 *
 *
 * --------------------------------------------------------------------------------------
 * COMPARING FILES:
 *	evolve_batch = dosfile.evolve  linux_file.evolve	<- compares files
 *
 *
 * --------------------------------------------------------------------------------------
 * KFORTH INTERPRETER:
 *	evolve_batch k				<- will read from stdin and compile/run it
 *	evolve_batch k	myprogram.kf		<- will compile/run myprogram.kf
 *
 * The KFORTH interpreter only contains the CORE KFORTH instructions (no OMOVE, CMOVE, etc...)
 * However three instructions were added:
 *
 *	.		print item on top of stack
 *	print		print item on top of stack (same as '.')
 *	.S		print data stack (do not deallocate elements)
 *
 * For example:
 *		{ 100 400 + print }	; will print 500 to stdout
 *
 * ----------------------------------------------------------------------
 * PICK RANDOM CREATURE:
 *	evolve_batch rc somefile.evolve 350 350	100 <- will pick random creature near (350,350)
 *	evolve_batch rc somefile.evolve		<- will pick random creature from file
 *
 * Extracts a random creature from the simulation file and prints out its data.
 *
 * You can pick a creature from the entire file, or from a smaller region
 *
 * The creature DNA is written to stdout
 * (meta data for creature is stored as comments)
 *
 * ----------------------------------------------------------------------
 * SIMULATE UNTIL ONE STRAIN LEFT
 *	evolve_batch 1s infile.evolve  outfile.evolve
 *
 * Simulate steps until the simulation consist of just 1 strain. Print out
 * the strain and the number of steps it took for this state to be reached.
 *
 * Output:
 *	2 120592
 *
 *	This means strain 2 is left after 120,592 steps
 *
 * Special conditions:
 *	0 0	if no creatures alive to start with
 *	s 0	if only 1 strain to start with 's' is that strain
 *
 */

#include "evolve_simulator.h"

//
// Common includes
//
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define ASSERT(x)	assert(x)
#define MALLOC(x)	malloc(x)
#define CALLOC(n,s)	calloc(n,s)
#define REALLOC(p,s)	realloc(p,s)
#define FREE(x)		free(x)
#define STRDUP(x)	_strdup(x)

/*
 * Return a timestamp in seconds, the
 * actual value is not important.
 *
 */
static long time_stamp(void)
{
#ifdef __windows__
	DWORD t;
	t = GetTickCount() / 1000;
	return (long) t;
#else
	return (long) time(NULL);
#endif
}

static void time_stamp_str(char* timebuf)
{
	time_t curtime;

	curtime = time(NULL);
	// "Thu Nov 24 18:22:48 1986\n\0"
	//  01234567890123456789012345678
	//      0123456789012345
	strcpy(timebuf, ctime(&curtime)+4);
	timebuf[15] = '\0';
}

/***********************************************************************
 * Provide dummy implementations to link with the Evolve4 GUI code 
 * (Just the Simulation subdirectory).
 *
 */
extern "C" {
	void EvolveFileBridge_rewind()
	{
	}

	intptr_t EvolveFileBridge_read(char *buf, intptr_t reqlen)
	{
		return 0;
	}

	intptr_t EvolveFileBridge_write(const char *buf, intptr_t len)
	{
		return 0;
	}
}

//////////////////////////////////////////////////////////////////////

/*
 * Compute a wacky checksum value for a universe
 * Allows us to compare files to ensure they are
 * the same.
 */
static int check_sum(UNIVERSE *u)
{
	int x, y;
	UNIVERSE_GRID ugrid;
	int i, d;
	KFORTH_INTEGER value;

	ASSERT( u != NULL );

	value = 0;
	for(x=0; x < u->width; x++) {
		for(y=0; y < u->height; y++) {
			Universe_Query(u, x, y, &ugrid);
			switch(ugrid.type) {
			case GT_BLANK:
				value += (x+y) * 5;
				break;

			case GT_BARRIER:
				value += (x+y) * 12;
				break;

			case GT_ORGANIC:
				value += (x+y) * 7 + ugrid.u.energy;
				break;

			case GT_CELL:
				value += (x+y) * ugrid.u.cell->organism->energy;
				i = 7;
				for(d=0; d < ugrid.u.cell->kfm.dsp; d++) {
					value += ugrid.u.cell->kfm.data_stack[d] * i;
					i += 7;
				}
				break;

			case GT_SPORE:
				value += (x+y) * ugrid.u.spore->energy;
				break;

			default:
				ASSERT(0);
			}

			value = value & 0x00FFFFFF;
		}
	}

	value = value & 0x00FFFFFF;

	return (int)value;
}

static LONG_LONG check_sum_stack(CELL *c)
{
	LONG_LONG value, i;
	int d;

	i = 7;
	value = 0;
	for(d=0; d < c->kfm.dsp; d++) {
		value += c->kfm.data_stack[d] * i;
		i += 731;
		value = value & 0x00FFFFFF;
	}

	return value;
}

static void print_info(char *filename, UNIVERSE *u)
{
	UNIVERSE_INFORMATION uinfo;

	ASSERT( filename != NULL );
	ASSERT( u != NULL );

	Universe_Information(u, &uinfo);

	printf("filename         %s\n",		filename);
	printf("step             %lld\n",	u->step);
	printf("age              %lld\n",	u->age);
	printf("nborn            %lld\n",	u->nborn);
	printf("ndie             %lld\n",	u->ndie);

	printf("width            %d\n",		u->width);
	printf("height           %d\n",		u->height);
	printf("seed             %u\n",		u->seed);
	printf("norganism        %d\n",		u->norganism);
	printf("energy           %d\n",		uinfo.energy);
	printf("num_cells        %d\n",		uinfo.num_cells);
	printf("num_instructions %d\n",		uinfo.num_instructions);
	printf("call_stack_nodes %d\n",		uinfo.call_stack_nodes);
	printf("data_stack_nodes %d\n",		uinfo.data_stack_nodes);
	printf("num_organic      %d\n",		uinfo.num_organic);
	printf("num_spores       %d\n",		uinfo.num_spores);
	printf("num_sexual       %d\n",		uinfo.num_sexual);
	printf("grid_memory      %d\n",		uinfo.grid_memory);
	printf("cstack_memory    %d\n",		uinfo.cstack_memory);
	printf("dstack_memory    %d\n",		uinfo.dstack_memory);
	printf("program_memory   %d\n",		uinfo.program_memory);
	printf("organism_memory  %d\n",		uinfo.organism_memory);
	printf("spore_memory     %d\n",		uinfo.spore_memory);
	printf("check_sum        %d\n",         check_sum(u));
}

static void usage(const char *s)
{
	ASSERT( s != NULL );

	printf("\n");
	printf("Usage:\n");

	printf("       evolve_batch s <time-spec> <infile.evolve> <outfile.evolve>\n");
	printf("\n");

	printf("       evolve_batch sf <time-spec> <infile.evolve> <outfile.evolve>\n");
	printf("            (simulate forever, check-pointing every <time-spec> intervals)\n");
	printf("\n");

	printf("       evolve_batch t <infile.png> min max <outfile.txt>\n");
	printf("            (generate terrain file from image. min/max form the greyscale pixel inclusion range)\n");
	printf("\n");

	printf("       evolve_batch p <infile.evolve>\n");
	printf("\n");

	printf("       evolve_batch k <kforth_file>\n");
	printf("\n");

	printf("       evolve_batch = <file1.evolve> <file2.evolve>\n");
	printf("\n");

	printf("       evolve_batch rc <file.evolve> [x y]\n");
	printf("\n");

	printf("       evolve_batch 1s <infile.evolve> <outfile.evolve>\n");
	printf("\n");
	
	printf("VERSION: %s\n", Evolve_Version());

	printf("ERROR: %s\n", s);
	printf("\n");
}

/*
 * Print top element on stack ( n -- )
 */
static void kfop_print(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	ASSERT( kfm != NULL );

	if( kfm->dsp > 0 ) {
		value = kforth_data_stack_pop(kfm);
		printf("%d\n", value);
	}
}

/*
 * Print stack, but don't remove anything.
 */
static void kfop_print_stack(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	int i;

	ASSERT( kfm != NULL );

	if( kfm->dsp > 0 ) {
		for(i=0; i < kfm->dsp; i++) {
			printf("%d\n", kfm->data_stack[i]);
		}
	} else {
		printf("(empty)\n");
	}
}


static void kforth_interpreter(FILE *fp, const char *filename)
{
	char program_text[100 * 1024];
	char buf[1000];
	char errbuf[1000];
	KFORTH_PROGRAM *kfp;
	KFORTH_OPERATIONS *kfops;
	KFORTH_MACHINE *kfm;
	KFORTH_DISASSEMBLY *kfd;

	program_text[0] = '\0';
	while( fgets(buf, sizeof(buf), fp) != NULL ) {
		strcat(program_text, buf);
	}

	/*
	 * Compile program
	 */
	kfops = kforth_ops_make();

	kforth_ops_add(kfops, ".", 0, 0, kfop_print);
	kforth_ops_add(kfops, "print", 0, 0, kfop_print);
	kforth_ops_add(kfops, ".S", 0, 0, kfop_print_stack);

	kfp = kforth_compile(program_text, kfops, errbuf);
	if( kfp == NULL ) {
		fprintf(stderr, "%s: %s\n", filename, errbuf);
		exit(1);
	}

	/*
	 * Print the program
	 */
	kfd = kforth_disassembly_make(kfops, kfp, 80, 0);
	printf("-------------------- disassembly --------------------\n");
	printf("%s", kfd->program_text);
	printf("-----------------------------------------------------\n");
	printf("\n");

	kforth_disassembly_delete(kfd);

	/*
	 * Run the program
	 */
	kfm = kforth_machine_make();

	while( ! kforth_machine_terminated(kfm) ) {
		kforth_machine_execute(kfops, kfp, kfm, NULL);
	}

	/*
	 * Print the stack
	 */
	printf("STACK:\n");
	kfop_print_stack(kfops, kfp, kfm, NULL);
	printf("\n");

	kforth_machine_delete(kfm);
	kforth_delete(kfp);
	kforth_ops_delete(kfops);
}

static void print_information(char *filename)
{
	UNIVERSE *u;
	char errbuf[1000];

	ASSERT( filename != NULL );

	u = Universe_Read(filename, errbuf);
	if( u == NULL ) {
		usage(errbuf);
		exit(1);
	}

	print_info(filename, u);

	Universe_Delete(u);
}

enum { SM_TIME, SM_STEP, SM_AGE };

/*
 * Simulate about 1000 steps, then print status.
 * If 'end_step' >= 0, then stop simulating when we reach this step.
 *
 */
static void simulate_chunk(UNIVERSE *u, int step_mode, LONG_LONG end_val)
{
	long long end_age;
	long long start_births, start_deaths;
	int oenergy = 0;
	int ncells = 0;
	ORGANISM *o;

	ASSERT( u != NULL );
	
	start_births = u->nborn;
	start_deaths = u->ndie;

	end_age = u->age + 1000;
	while( u->age < end_age ) {
		if( step_mode == SM_STEP ) {
			if( u->step >= end_val )
				break;
		} else if( step_mode == SM_AGE ) {
			if( u->age >= end_val )
				break;
		}
		Universe_Simulate(u);
	}
	
	for(o=u->organisms; o; o=o->next) {
		ncells += o->ncells;
		oenergy += o->energy;
	}

	printf("Age: %lld, Step: %lld, Organisms: %4d, Cells: %d, Energy: %d, Born: %lld, Died: %lld (%+lld)\n",
					u->age, u->step, u->norganism, ncells, oenergy, u->nborn,
		 			  	u->ndie, (u->ndie - start_deaths));
}

static void do_simulate(int forever, char *time_spec, char *in_filename, char *out_filename)
{
	char errbuf[1000];
	int value, tspec;
	char unit;
	const char *unit_desc;
	int result;
	UNIVERSE *u;
	LONG_LONG start_val, end_val;
	long start_seconds, end_seconds = 0, now;
	int step_mode;
	char nowbuf[100];
	

	ASSERT( time_spec != NULL );
	ASSERT( in_filename != NULL );
	ASSERT( out_filename != NULL );

	tspec = atoi(time_spec);

	unit = time_spec[ strlen(time_spec)-1 ];

	switch( unit ) {
	case 'h':
		step_mode = SM_TIME;
		unit_desc = "hours";
		value = tspec * 60 * 60;
		break;

	case 'm':
		step_mode = SM_TIME;
		unit_desc = "minutes";
		value = tspec * 60;
		break;

	case 's':
		step_mode = SM_TIME;
		unit_desc = "seconds";
		value = tspec * 1;
		break;

	case 'u':
		step_mode = SM_STEP;
		unit_desc = "steps";
		value = tspec * 1;
		break;

	case 'a':
		step_mode = SM_AGE;
		unit_desc = "ages";
		value = tspec * 1;
		break;
		
	default:
		usage("Time spec unit must be 'h', 'm', 's', 'u' or 'a'.");
		exit(1);
	}

	printf("Input:  %s\n", in_filename);
	printf("Output: %s\n", out_filename);
	if( forever ) {
		printf("About to simulate universe for FOREVER.\n");
		printf("Checkpoint interval is every %d %s.\n", tspec, unit_desc);
	} else {
		printf("About to simulate universe for %d %s...\n", tspec, unit_desc);
	}

#if 0
	/*
	 * Test to make sure the output file can be created
	 * It screws up my perl script so we dont do this test.
	 */
{
	FILE *fp;
	fp = fopen(out_filename, "wb");
	if( fp == NULL ) {
		sprintf(errbuf, "unable to create/write to '%s'(%s)",
					out_filename, strerror(errno));
		usage(errbuf);
		exit(1);
	}
	fclose(fp);
}
#endif

	/*
	 * Okay here we go... Open input file for simulating
	 *
	 */
	u = Universe_Read(in_filename, errbuf);
	if( u == NULL ) {
		usage(errbuf);
		exit(1);
	}
	
do {
		
	time_stamp_str(nowbuf);

	printf("%s ---------- BEGIN ----------\n", nowbuf);

	if( step_mode == SM_STEP ) {
		ASSERT( unit == 'u' );
		start_val = u->step;
		end_val = start_val + value;
	} else if( step_mode == SM_AGE ) {
		ASSERT( unit == 'a' );
		start_val = u->age;
		end_val = start_val + value;
	} else {
		ASSERT( step_mode == SM_TIME );
		start_seconds = time_stamp();
		end_seconds = start_seconds + value;
		end_val = 0;
	}
	
	for(;;) {
		if( step_mode == SM_STEP ) {
			if( u->step >= end_val ) {
				break;
			}
		} else if( step_mode == SM_AGE ) {
			if( u->age >= end_val ) {
				break;
			}
		} else {
			ASSERT( step_mode == SM_TIME );

			now = time_stamp();
			if( now >= end_seconds ) {
				break;
			}
		}
		simulate_chunk(u, step_mode, end_val);
	}

	time_stamp_str(nowbuf);

	printf("%s ---------- END ----------\n", nowbuf);

	result = Universe_Write(u, out_filename, errbuf);
	if( ! result ) {
		usage(errbuf);
	}

	if( forever ) {
		printf("Wrote %s. Resuming simulating...\n", out_filename);
	}
		
} while( forever );

	Universe_Delete(u);
}

static void simulate(char *time_spec, char *in_filename, char *out_filename)
{
	do_simulate(0, time_spec, in_filename, out_filename);
}

static void simulateForever(char *time_spec, char *in_filename, char *out_filename)
{
	do_simulate(1, time_spec, in_filename, out_filename);
}

static const char *grid_type_to_string(int type)
{
	switch( type ) {
	case GT_BLANK:
		return "blank";

	case GT_BARRIER:
		return "barrier";

	case GT_ORGANIC:
		return "organic";

	case GT_CELL:
		return "cell";

	case GT_SPORE:
		return "spore";

	default:
		ASSERT(0);
	}

	return "NOTREACHED";
}

static void compare_universes(char *file1, char *file2)
{
	UNIVERSE *u1, *u2;
	UNIVERSE_GRID ugrid1, ugrid2;
	char errbuf[1000];
	const char *type_string1, *type_string2;
	int x, y;
	int diffs;
	CELL *c1, *c2;
	ORGANISM *o1, *o2;
	SPORE *s1, *s2;

	ASSERT( file1 != NULL );
	ASSERT( file2 != NULL );

	u1 = Universe_Read(file1, errbuf);
	if( u1 == NULL ) {
		usage(errbuf);
		exit(1);
	}

	u2 = Universe_Read(file2, errbuf);
	if( u2 == NULL ) {
		usage(errbuf);
		exit(1);
	}

	printf("---------- FILE 1 ----------\n");
	print_info(file1, u1);
	printf("----------------------------\n\n");

	printf("---------- FILE 2 ----------\n");
	print_info(file2, u2);
	printf("----------------------------\n\n");

	if( u1->width != u2->width ) {
		printf("width's are not the same. cannot diff.\n");
		exit(1);
	}

	if( u1->height != u2->height ) {
		printf("height's are not the same. cannot diff.\n");
		exit(1);
	}

	diffs = 0;

	for(x=0; x < u1->width; x++) {
		for(y=0; y < u1->height; y++) {
			Universe_Query(u1, x, y, &ugrid1);
			Universe_Query(u2, x, y, &ugrid2);

			if( ugrid1.type != ugrid2.type ) {
				type_string1 = grid_type_to_string(ugrid1.type);
				type_string2 = grid_type_to_string(ugrid2.type);

				printf("(%d, %d) type mismatch. '%s' != '%s'\n",
					x, y, type_string1, type_string2);

				diffs++;

				continue;
			}

			switch( ugrid1.type ) {
			case GT_ORGANIC:
				if( ugrid1.u.energy != ugrid2.u.energy ) {
					printf("(%d, %d) ORGANIC energy mismatch. '%d' != '%d'\n",
						x, y, ugrid1.u.energy, ugrid2.u.energy);
					diffs++;
					continue;
				}
				break;

			case GT_CELL:
				c1 = ugrid1.u.cell;
				c2 = ugrid2.u.cell;
				o1 = c1->organism;
				o2 = c2->organism;
				if( o1->energy != o2->energy ) {
					printf("(%d, %d) ORGANISM energy mismatch. '%d' != '%d'\n",
						x, y, o1->energy, o2->energy);
					diffs++;
					continue;
				}
				if( check_sum_stack(c1) != check_sum_stack(c2) ) {
					printf("(%d, %d) CELL stack mismatch.\n", x, y);
					diffs++;
					continue;
				}
				break;

			case GT_SPORE:
				s1 = ugrid1.u.spore;
				s2 = ugrid2.u.spore;
				if( s1->energy != s2->energy ) {
					printf("(%d, %d) SPORE energy mismatch. '%d' != '%d'\n",
						x, y, s1->energy, s2->energy);
					diffs++;
					continue;
				}
				break;
			}
		}
	}

	if( diffs == 0 ) {
		printf("Files are the same\n");
	}

	Universe_Delete(u1);
	Universe_Delete(u2);
}

//////////////////////////////////////////////////////////////////////
static void print_prolog(FILE *fp, int width, int height)
{
	fprintf(fp, "# PHOTON ASCII\n");
	fprintf(fp, "\n");
	fprintf(fp, "struct UNIVERSE {\n");
	fprintf(fp, "        SEED\n");
	fprintf(fp, "        STEP\n");
	fprintf(fp, "        AGE\n");
	fprintf(fp, "        CURRENT_CELL { X Y }\n");
	fprintf(fp, "        NEXT_ID\n");
	fprintf(fp, "        NBORN\n");
	fprintf(fp, "        NDIE\n");
	fprintf(fp, "        WIDTH\n");
	fprintf(fp, "        HEIGHT\n");
	fprintf(fp, "        G0\n");
	fprintf(fp, "        KEY\n");
	fprintf(fp, "        MOUSE_X\n");
	fprintf(fp, "        MOUSE_Y\n");
	fprintf(fp, "        S0[N] { V }\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "struct BARRIER[N] {\n");
	fprintf(fp, "        X\n");
	fprintf(fp, "        Y\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "UNIVERSE 0          # seed\n");
	fprintf(fp, "         0          # step\n");
	fprintf(fp, "         0          # age\n");
	fprintf(fp, "         -1 -1      # current cell location (x,y)\n");
	fprintf(fp, "         0          # next id\n");
	fprintf(fp, "         0 0            # number births, deaths\n");
	fprintf(fp, "         %d %d    # dimensions: width x height\n", width, height);
	fprintf(fp, "         0          # global register G0\n");
	fprintf(fp, "         0          # key\n");
	fprintf(fp, "         -1         # mouse_x\n");
	fprintf(fp, "         -1         # mouse_y\n");
	fprintf(fp, "       { 0 0 0 0 0 0 0 0 }  # S0's for each strain\n");
	fprintf(fp, "\n");
	fprintf(fp, "\n");
}

static void print_barrier_block(FILE *fp, int counter, int x, int y)
{
	int CHUNK_SIZE = 1000;

	if( counter != 0 && (counter % CHUNK_SIZE) == 0 ) {
		fprintf(fp, "}\n");
	}

	if( counter == 0 || (counter % CHUNK_SIZE) == 0 ) {
		fprintf(fp, "BARRIER {\n");
	}
	fprintf(fp, "  %d %d\n", x, y);
}

static void print_trailer(FILE *fp)
{
	fprintf(fp, "}\n");
}

/*
 *
 * Returns 0 - error, 1 - success
 */
static int image2terrain(const char *input_filename, int min, int max, const char *output_filename, char *errbuf)
{
	int x, y, n;
	int width, height;
	int counter;
	unsigned char *data;
	FILE *fp;
	int pixel;

	fp = fopen(output_filename, "w");
	if( fp == NULL )
	{
		sprintf(errbuf, "%s: cannot open. %s", input_filename, strerror(errno));
		return 0;
	}

	data = stbi_load(input_filename, &width, &height, &n, 1);
	if( data == NULL ) {
		sprintf(errbuf, "%s: stbi_load failed", input_filename);
		fclose(fp);	
		return 0;
	}

	print_prolog(fp, width, height);

	counter = 0;

	for(x=0; x < width; x++) {
		for(y=0; y < height; y++) {
			pixel = data[y * width + x];
			if( pixel >= min && pixel <= max ) {
				print_barrier_block(fp, counter, x, y);
				counter += 1;
			}
		}
	}

	stbi_image_free(data);

	print_trailer(fp);

	fclose(fp);	

	return 1;
}

//////////////////////////////////////////////////////////////////////

/*
 * Main program
 *
 */
int main(int argc, char* argv[])
{
	char errbuf[1000];
	const char *filename;
	int success;
	FILE *fp;

	if( argc == 1 ) {
		usage("No arguments.");
		exit(1);
	}

	if( strcmp(argv[1], "p") == 0 ) {
		if( argc != 3 ) {
			usage("'p' option must be followed by a simulation filename.");
			exit(1);
		}

		print_information(argv[2]);

	} else if( strcmp(argv[1], "s") == 0 ) {
		if( argc != 5 ) {
			usage("'s' option must be followed by exactly 3 arguments.");
			exit(1);
		}
		simulate(argv[2], argv[3], argv[4]);
		
	} else if( strcmp(argv[1], "sf") == 0 ) {
			if( argc != 5 ) {
			 usage("'sf' option must be followed by exactly 3 arguments.");
			 exit(1);
		 }
		 simulateForever(argv[2], argv[3], argv[4]);

	} else if( strcmp(argv[1], "k") == 0 ) {
		if( argc > 3 ) {
			usage("'k' option must by followed by a kforth file, or nothing.");
			exit(1);
		} else if( argc < 3 ) {
			fp = stdin;
			filename = "stdin";
		} else {
			filename = argv[2];
			fp = fopen(filename, "r");
			if( fp == NULL ) {
				sprintf(errbuf, "%s: %s", filename, strerror(errno));
				usage(errbuf);
				exit(1);
			}
		}
		kforth_interpreter(fp, filename);

		if( fp != stdin )
			fclose(fp);

	} else if( strcmp(argv[1], "=") == 0 ) {
		if( argc != 4 ) {
			usage("'=' option must be followed by exactly 2 arguments.");
			exit(1);
		}
		compare_universes(argv[2], argv[3]);

	} else if( strcmp(argv[1], "t") == 0 ) {
		if( argc != 6 ) {
			usage("'t' option must be followed by exactly 4 arguments.");
			exit(1);
		}
		success = image2terrain(argv[2], atoi(argv[3]), atoi(argv[4]), argv[5], errbuf);
		if( !success )
		{
			usage(errbuf);
			exit(1);
		}

	} else {
		usage("First argument must be 'p' or 's' or 'sf' or 'k' or '='.");
		exit(1);
	}

	exit(0);
}
