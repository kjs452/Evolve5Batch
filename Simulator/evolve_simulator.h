#ifndef _EVOLVE_SIMULATOR_H
#define _EVOLVE_SIMULATOR_H

/*
 * Copyright (c) 2006 Stauffer Computer Consulting
 */

/***********************************************************************
 * evolve_simulator:
 *
 * Interface for the evolve simulator. Clients wanting to
 * create a simulation, and simulate it, include this file.
 *
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 * KFORTH COMMON
 */
typedef int16_t KFORTH_INTEGER;

/***********************************************************************
 * KFORTH PROGRAM
 *
 *
 * kfp->block[cb] -------+
 *                       |
 *              len      v     i2     i3    i4    i5      i6
 *            +------+------+------+------+------+------+------+
 *            |  6   |OMOVE | dup  | 1233 | call | -123 |?loop |
 *            +------+------+------+------+------+------+------+
 *               -1     0      1      2      3       4      5
 *
 *
 * The block pointers are shifted by 1. block[cb][-1] is the length
 *
 * Use: kfp->block[pc][cb] is the way to access the program
 * Use: kfp->block[pc][-1] to get the length
 *
 * 'nprotected' should be: 0 <= nprotected <= nblocks.
 * This fields designates the first 'nprotected' code blocks as "protected".
 *
 */
typedef struct {
	int					nblocks;
	int					nprotected;
	KFORTH_INTEGER		**block;
} KFORTH_PROGRAM;

/***********************************************************************
 * KFORTH DISASSEMBLY
 */
typedef struct {
	int	cb;
	int	pc;		/* -1 means label definition */
	int	start_pos;
	int	end_pos;
} KFORTH_DISASSEMBLY_POS;

typedef struct {
	char					*program_text;
	int						pos_len;
	KFORTH_DISASSEMBLY_POS	*pos;
} KFORTH_DISASSEMBLY;

/***********************************************************************
 * KFORTH SYMBOL TABLE - for faster parsing of kforth programs
 */
#define KF_HASH_SIZE	(127*127)
#define KF_CHAIN_LEN	3

typedef struct {
	int16_t list[ KF_CHAIN_LEN ];		// use -1 to terminate list
} KFORTH_SYMLIST;

typedef struct {
	KFORTH_SYMLIST hash[ KF_HASH_SIZE ];
} KFORTH_SYMTAB;

/***********************************************************************
 * KFORTH MACHINE
 */
typedef struct {
	int16_t		pc;
	int16_t		cb;
} KFORTH_LOC;

#define KF_MAX_CALL	64		// maximum call stack size
#define KF_MAX_DATA	64		// maximum data stack size

typedef struct {
	KFORTH_LOC			loc;							// program location
	KFORTH_INTEGER		R[10];							/* R0 - R9 */
	int16_t				csp;							// call stack pointer
	int16_t				dsp;							// data stack pointer
	KFORTH_LOC			call_stack[ KF_MAX_CALL ];
	KFORTH_INTEGER		data_stack[ KF_MAX_DATA ];
} KFORTH_MACHINE;

/***********************************************************************
 * KFORTH OPERATIONS
 */
#define KFORTH_OPS_LEN	250	/* maximum number of instructions supported */

typedef struct kforth_operations KFORTH_OPERATIONS;

typedef void (*KFORTH_FUNCTION)(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data);

typedef struct kforth_operation {
	const char			*name;
	KFORTH_FUNCTION		func;
	int					key;		// EvolveOperations() assigns these
	int					in;			// number of input arguments expected on data stack
	int					out;		// number of output arguments expected on data stack
} KFORTH_OPERATION;

struct kforth_operations {
	int					count;						// number of enrties in 'table'
	int					nprotected;					// number of protected instructions (from start of table)
	KFORTH_OPERATION	table[KFORTH_OPS_LEN];		// a table of kforth instructions
};

/***********************************************************************
 * KFORTH MUTATE OPTIONS
 */
typedef struct {
	int	prob_mutate_codeblock;
	int	prob_duplicate;
	int	prob_delete;
	int	prob_insert;
	int	prob_transpose;
	int	prob_modify;
	int	max_code_blocks;
	int	max_apply;
	int merge_mode;
	int xlen;
	int protected_codeblocks;			// code blocks from start which are marked protected. this is a user setting
										// which eventually gets propogated into kfp->nprotected.
} KFORTH_MUTATE_OPTIONS;

#define PROBABILITY_SCALE	10000
#define MUTATE_MAX_APPLY_LIMIT	10		/* upper limit for the max_apply setting */

/***********************************************************************
 * KFORTH INSTRUCTION HELP
 * #define MASK_K	0x01	 kforth core instructions
 * #define MASK_C	0x02	 cell instruction
 * #define MASK_F	0x04	 find instructions
 */
typedef struct {
	int			mask;
	const char	*instruction;
	const char	*symbol;
	const char	*comment;
	const char	*description;
} KFORTH_IHELP;

/*
 * help.cpp
 */
extern int Kforth_Instruction_Help_len;
extern KFORTH_IHELP *Kforth_Instruction_Help;

/***********************************************************************
 * RANDOM NUMBER GENERATOR
 */
#define	EVOLVE_DEG4	63
#define	EVOLVE_SEP4	1

typedef struct {
	uint32_t 	fidx;			/* front index */
 	uint32_t 	ridx;			/* rear index */
	uint32_t 	state[ EVOLVE_DEG4 ];
} EVOLVE_RANDOM;

typedef int64_t				LONG_LONG;
typedef struct universe		UNIVERSE;
typedef struct organism		ORGANISM;
typedef struct cell			CELL;

/***********************************************************************
 * CELL
 *
 */
struct cell {
	short			color;			/* used by kill_dead_cells() algorithm */
	KFORTH_INTEGER	mood;
	KFORTH_INTEGER	message;
	KFORTH_MACHINE	kfm;
	int				x;
	int				y;
	CELL			*next;
	ORGANISM		*organism;	/* pointer to my organism */
	CELL			*u_next;
	CELL			*u_prev;
};

/***********************************************************************
 * ORGANISM
 *
 */
struct organism {
	LONG_LONG		id;				/* unique-id for this organism */
	LONG_LONG		parent1;		/* unique-id for this organism's parent */
	LONG_LONG		parent2;		/* unique-id for this organism's parent */
	int				generation;		/* generation from parent zero */
	int				energy;			/* amount of energy the organism has */
	int				age;			/* how old this organism is */
	int				strain;			/* what strain does this organism belong to? */
	int				oflags;			/* various organism flags */
	int				sim_count;		/* down counter until all cells simulated */
	KFORTH_PROGRAM	program;
	int				ncells;			/* number of cells */
	CELL			*cells;			/* linked list of cells in the organism */
	ORGANISM		*next;
	ORGANISM		*prev;
};

#define ORGANISM_FLAG_RADIOACTIVE	0x00000001		/* radioactive dye marker */
#define ORGANISM_FLAG_READWRITE		0x00000002		/* code block changed due to READ/WRITE */

/***********************************************************************
 * SPORE
 */
typedef struct spore {
	int				energy;
	int				strain;		/* strain of organism that created me */
	int				sflags;		/* various spore flags */
	LONG_LONG		parent;		/* parent-id that created me */
	KFORTH_PROGRAM	program;	/* program from parent */
} SPORE;

#define SPORE_FLAG_RADIOACTIVE		0x00000001		/* radioactive dye marker */

/***********************************************************************
 * GRID
 */
#define EVOLVE_MAX_BOUNDS	3000
#define EVOLVE_MIN_BOUNDS	5
#define EVOLVE_MAX_STRAINS	8

typedef enum {
	GT_BLANK,
	GT_BARRIER,
	GT_ORGANIC,
	GT_CELL,
	GT_SPORE,
} GRID_TYPE;

typedef struct universe_grid {
	short			type;					/* GRID_TYPE */
	KFORTH_INTEGER	odor;
	union {
		int			energy;
		CELL		*cell;
		SPORE		*spore;
	} u;
} UNIVERSE_GRID;

/**********************************************************************
 * SIMULATION and STRAIN OPTIONS
 */
typedef struct {
	int		mode;			// 0=normal energy mode, 1=cell energy mode
} SIMULATION_OPTIONS;

typedef struct {
	int		enabled;			// indicates if this strain is enabled or not
	char	name[100];			// strain name
	int		look_mode;			// LOOK: 0=default/original. 1=cannot see thru self.
	int		eat_mode;			// EAT: 0=default/original. 1=can eat self, killing self.
	int		make_spore_mode;	// MAKE-SPORE: 0=default/original. sexual only. asexual only.
	int		make_spore_energy;	// MAKE-SPORE Energy
	int		cmove_mode;			// CMOVE: 0=default.
	int		omove_mode;			// OMOVE 0=default.
	int		grow_mode;			// GROW: 0=default, 1=can grow to an occupied square shifting self cells
	int		grow_energy;		// GROW Energy
	int		grow_size;			// GROW Max Size
	int		rotate_mode;		// ROTATE: 0=default 90 deg. 1=45 deg (if diagonal) must use center point of orgnism
	int		cshift_mode;		// CSHIFT: 0=default.
	int		make_organic_mode;	// MAKE-ORGANIC: 0=default
	int		make_barrier_mode;	// MAKE-BARRIER mode
	int		exude_mode;			// EXUDE mode
	int		shout_mode;			// SHOUT mode
	int		spawn_mode;			// SPAWN mode
	int		listen_mode;		// LISTEN mode
	int		broadcast_mode;		// BROADCAST mode
	int		say_mode;			// SAY mode
	int		send_energy_mode;	// SEND_ENERGY mode
	int		read_mode;
	int		write_mode;
	int		key_press_mode;
	int		send_mode;
} STRAIN_OPTIONS;

/***********************************************************************
 * UNIVERSE
 *
 */
struct universe {
	uint32_t				seed;			/* random seed -- initially used */
	LONG_LONG				step;			/* how many steps */
	LONG_LONG				age;			/* how many steps 2 */
	LONG_LONG				next_id;		/* next organism id to assign */
	int						norganism;		/* # of organisms in universe */
	int						strpop[8];		/* # of organisms by strain */
	LONG_LONG				nborn;			/* # of organisms born in this simulation */
	LONG_LONG				ndie;			/* # of organisms died in this simulation */
	EVOLVE_RANDOM			er;				/* random number generator state */
	SIMULATION_OPTIONS 		so;
	STRAIN_OPTIONS			strop[8];		/* options pertaining to a strain */
	KFORTH_OPERATIONS		kfops[8];		/* list of instructions available to this strain */
	KFORTH_MUTATE_OPTIONS	kfmo[8];		/* mutations probabilities can be adjusted per strain */
	ORGANISM				*organisms;
	ORGANISM				*selected_organism;
	int						width;
	int						height;
	UNIVERSE_GRID			*grid;
	CELL					*current_cell;
	CELL					*cells;
	KFORTH_INTEGER			G0;				/* universe-wide global variable */
	int						key;			/* KEY-PRESS */
	int						mouse_x;		/* MOUSE-POS */
	int						mouse_y;		/* MOUSE-POS */
	KFORTH_INTEGER			S0[8];			/* strain-wide global variable */
	int						barrier_flag;	/* set whenever the barrier layer changes, clients can clear, not saved */
};

typedef struct {
	int	energy;
	int	num_cells;
	int	num_instructions;
	int	call_stack_nodes;
	int	data_stack_nodes;
	int	num_organic;
	int	num_spores;
	int	num_sexual;	// # of organisms that have 2 parents

	int	spore_energy;
	int	organic_energy;

	int	grid_memory;
	int	cstack_memory;
	int	dstack_memory;
	int	program_memory;
	int	organism_memory;
	int	spore_memory;

	int	strain_population[EVOLVE_MAX_STRAINS];
	int	radioactive_population[EVOLVE_MAX_STRAINS];

} UNIVERSE_INFORMATION;

typedef struct {
	char					name[100];
	char					seed_file[1000];
	int						energy;
	int						population;
	char					description[10000];
	STRAIN_OPTIONS			strop;
	KFORTH_MUTATE_OPTIONS	kfmo;
	KFORTH_OPERATIONS		kfops;
} STRAIN_PROFILE;

typedef struct {
	int				profile_idx;			// index into 'strain_profiles', use -1 to indicate nothing set.
	int				energy;
	int				population;
	char			seed_file[1000];
} EVOLVE_DFLT;

typedef struct {
	SIMULATION_OPTIONS	so;
	char				evolve_batch_path[1000];
	char				evolve_3d_path[1000];
	char				help_path[1000];
	int					width;
	int					height;
	int					want_barrier;
	char				terrain_file[1000];
	EVOLVE_DFLT			dflt[8];
	int					nprofiles;
	STRAIN_PROFILE		*strain_profiles;
} EVOLVE_PREFERENCES;

/*
 * options for a new universe
 */
typedef struct
{
	int32_t				seed;
	int					width;
	int					height;
	int					want_barrier;
	char				terrain_file[1000];
	SIMULATION_OPTIONS	so;
	STRAIN_PROFILE		strain_profiles[8];
} NEW_UNIVERSE_OPTIONS;

/***********************************************************************
 * PROTOTYPES:
 */

/*
 * kforth_mutate.cpp
 */
extern KFORTH_MUTATE_OPTIONS *kforth_mutate_options_make(
				int max_code_blocks,
				int max_apply,
				double prob_mutate_codeblock,
				double prob_duplicate,
				double prob_delete,
				double prob_insert,
				double prob_transpose,
				double prob_modify,
				int merge_mode,
				int xlen,
				int protected_codeblocks );

extern void kforth_mutate_options_copy2(KFORTH_MUTATE_OPTIONS *kfmo, KFORTH_MUTATE_OPTIONS *kfmo2);
extern KFORTH_MUTATE_OPTIONS *kforth_mutate_options_copy(KFORTH_MUTATE_OPTIONS *kfmo);
extern void		kforth_mutate_options_defaults(KFORTH_MUTATE_OPTIONS *kfmo);
extern void		kforth_mutate_options_delete(KFORTH_MUTATE_OPTIONS *kfmoxo);

extern void		kforth_mutate(KFORTH_OPERATIONS *kfops,
					KFORTH_MUTATE_OPTIONS *kfmo,
					EVOLVE_RANDOM *er,
					KFORTH_PROGRAM *kfp);

extern void		kforth_mutate_cb(KFORTH_OPERATIONS *kfops,
					KFORTH_MUTATE_OPTIONS *kfmo,
					EVOLVE_RANDOM *er,
					KFORTH_INTEGER **block);

extern void kforth_merge2(EVOLVE_RANDOM *er,
						  KFORTH_MUTATE_OPTIONS *kfmo,
						  KFORTH_PROGRAM *kfp1,
						  KFORTH_PROGRAM *kfp2,
						  KFORTH_PROGRAM *kfp);

extern KFORTH_PROGRAM	*kforth_merge(EVOLVE_RANDOM *er,
									  KFORTH_MUTATE_OPTIONS *kfmo,
									  KFORTH_PROGRAM *kfp1,
									  KFORTH_PROGRAM *kfp2);

extern void kforth_copy2(KFORTH_PROGRAM *kfp, KFORTH_PROGRAM *kfp2);
extern KFORTH_PROGRAM	*kforth_copy(KFORTH_PROGRAM *kfp);
extern void				kforth_program_init(KFORTH_PROGRAM *kfp);
extern void				kforth_program_deinit(KFORTH_PROGRAM *kfp);
extern int				kforth_program_cblen(KFORTH_PROGRAM *kfp, int cb);

/*
 * kforth_execute.cpp
 */
extern void		kforth_machine_init(KFORTH_MACHINE *kfm);
extern void		kforth_machine_deinit(KFORTH_MACHINE *kfm);
extern KFORTH_MACHINE	*kforth_machine_make();
extern void		kforth_machine_delete(KFORTH_MACHINE *kfm);
extern void		kforth_machine_copy2(KFORTH_MACHINE *kfm, KFORTH_MACHINE *kfm2);
extern KFORTH_MACHINE	*kforth_machine_copy(KFORTH_MACHINE *kfm);
extern void		kforth_machine_execute(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *program, KFORTH_MACHINE *kfm, void *client_data);
extern void		kforth_machine_reset(KFORTH_MACHINE *kfm);
extern int		kforth_machine_terminated(KFORTH_MACHINE *kfm);
extern void		kforth_machine_terminate(KFORTH_MACHINE *kfm);

extern void kforth_ops_init(KFORTH_OPERATIONS *kfops);
extern KFORTH_OPERATIONS *kforth_ops_make(void);
extern void		kforth_ops_delete(KFORTH_OPERATIONS *kfops);
extern void		kforth_ops_add2(KFORTH_OPERATIONS *kfops, KFORTH_OPERATION *kfop);
extern void		kforth_ops_add(KFORTH_OPERATIONS *kfops,
						const char *name, int in, int out, KFORTH_FUNCTION func);
extern int		kforth_ops_max_opcode(KFORTH_OPERATIONS *kfops);
extern int		kforth_ops_find(KFORTH_OPERATIONS *kfops, const char *name);
extern void		kforth_ops_del(KFORTH_OPERATIONS *kfops, const char *name);
extern KFORTH_OPERATION* kforth_ops_get(KFORTH_OPERATIONS *kfops, int idx);
extern void		kforth_ops_set_protected(KFORTH_OPERATIONS *kfops, const char *name);
extern void		kforth_ops_set_unprotected(KFORTH_OPERATIONS *kfops, const char *name);

extern KFORTH_INTEGER	kforth_data_stack_pop(KFORTH_MACHINE *kfm);
extern void		kforth_data_stack_push(KFORTH_MACHINE *kfm, KFORTH_INTEGER value);
extern void		kforth_call_stack_push(KFORTH_MACHINE *kfm, int cb, int pc);

/*
 * kforth_compiler.cpp
 */
extern KFORTH_PROGRAM	*kforth_compile(const char *program_text,
					KFORTH_OPERATIONS *kfops, char *errbuf);
extern void		kforth_delete(KFORTH_PROGRAM *kfp);
extern int		kforth_program_length(KFORTH_PROGRAM *kfp);
extern int		kforth_program_size(KFORTH_PROGRAM *kfp);

extern KFORTH_DISASSEMBLY *kforth_disassembly_make(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, int width, int want_cr);
extern void		kforth_disassembly_delete(KFORTH_DISASSEMBLY *kfd);

extern int		kforth_program_find_symbol(const char *program, const char *symbol);
extern int		kforth_remap_instructions(KFORTH_OPERATIONS* kfops1, KFORTH_OPERATIONS* kfops2, KFORTH_PROGRAM *kfp);
extern int		kforth_remap_instructions_cb(KFORTH_OPERATIONS* kfops1, KFORTH_OPERATIONS* kfops2, KFORTH_INTEGER *block);
extern KFORTH_SYMTAB *kforth_symtab_make(KFORTH_OPERATIONS *kfops);
extern void		kforth_symtab_delete(KFORTH_SYMTAB *kfst);
extern KFORTH_PROGRAM *kforth_compile_kfst(const char *program_text, KFORTH_SYMTAB *kfst, KFORTH_OPERATIONS *kfops, char *errbuf);

extern char *kforth_metadata_comment_make(int strain, STRAIN_OPTIONS *strop, KFORTH_MUTATE_OPTIONS *kfmo, KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp);
extern void kforth_metadata_comment_delete(char *str);

/*
 * cell.cpp
 */
extern CELL	*Cell_Neighbor(UNIVERSE *u, CELL *cell, int xoffset, int yoffset);
extern void	Cell_delete(CELL *c);

/*
 * 8 directions map as follows:
 *
 *	index	dir
 *	------	--------
 *	  0		( 0, -1)
 *	  1		( 1, -1)
 *	  2		( 1,  0)
 *	  3		( 1,  1)
 *	  4		( 0,  1)
 *	  5		(-1,  1)
 *	  6		(-1,  0)
 *	  7		(-1, -1)
 *
 */
typedef struct {
	int strain;

	// vision
	int	what;		
	int	dist;
	int size;
	int energy;

	// sound
	int mood;
	int message;

	// odor
	int odor;

} CELL_SENSE_DATA_ITEM;

typedef struct {
	CELL_SENSE_DATA_ITEM dirs[8];
	int odor;		// odor at (0,0)
} CELL_SENSE_DATA;

extern void	Universe_CellSensoryData(UNIVERSE *u, CELL *cell, CELL_SENSE_DATA *csd);

/*
 * organism.cpp
 */
extern ORGANISM	*Organism_Make(
				int x, int y,
				int strain, int energy,
				KFORTH_OPERATIONS *kfops,
				int protected_codeblocks,
				const char *program_text,
				char *errbuf );

extern void		Organism_delete(ORGANISM *o);

/*
 * spore.cpp
 */
extern SPORE	*Spore_make(KFORTH_PROGRAM *program, int energy, LONG_LONG parent, int strain);
extern void		Spore_delete(SPORE *spore);
extern void		Spore_fertilize(UNIVERSE *u, ORGANISM *o, SPORE *spore, int x, int y, int energy);

/*
 * universe.cpp
 */
extern UNIVERSE	*Universe_Make(uint32_t seed, int width, int height);
extern void		Universe_Delete(UNIVERSE *u);
extern void		Universe_Simulate(UNIVERSE *u);
extern void		Universe_Information(UNIVERSE *u, UNIVERSE_INFORMATION *uinfo);


extern void		Universe_SetBarrier(UNIVERSE *u, int x, int y);
extern void		Universe_ClearBarrier(UNIVERSE *u, int x, int y);
extern GRID_TYPE	Universe_Query(UNIVERSE *u, int x, int y, UNIVERSE_GRID *ugrid);

extern void		Universe_SelectOrganism(UNIVERSE *u, ORGANISM *o);
extern void		Universe_ClearSelectedOrganism(UNIVERSE *u);
extern ORGANISM		*Universe_GetSelection(UNIVERSE *u);

extern ORGANISM		*Universe_DuplicateOrganism(ORGANISM *osrc);
extern ORGANISM		*Universe_CopyOrganism(UNIVERSE *u);
extern ORGANISM		*Universe_CutOrganism(UNIVERSE *u);
extern void		Universe_PasteOrganism(UNIVERSE *u, ORGANISM *o);
extern void		Universe_FreeOrganism(ORGANISM *o);

/*
 * These API's applie to a COPIED_ORGANISM which includes the
 * organism and its strain properties.
 */
typedef struct {
	ORGANISM				*o;
	KFORTH_OPERATIONS		kfops;
	KFORTH_MUTATE_OPTIONS	kfmo;
	STRAIN_OPTIONS			strop;
} COPIED_ORGANISM;

extern COPIED_ORGANISM *Universe_CopyOrganismCo(UNIVERSE *u);
extern COPIED_ORGANISM *Universe_CutOrganismCo(UNIVERSE *u);
extern void Universe_PasteOrganismCo(UNIVERSE *u, COPIED_ORGANISM *co);
extern void Universe_FreeOrganismCo(COPIED_ORGANISM *co);

extern void		Universe_ClearTracers(UNIVERSE *u);
extern void		Universe_SetSporeTracer(SPORE *spore);
extern void		Universe_SetOrganismTracer(ORGANISM *organism);
extern void		Universe_ClearSporeTracer(SPORE *spore);
extern void		Universe_ClearOrganismTracer(ORGANISM *organism);

extern GRID_TYPE	Grid_GetPtr(UNIVERSE *u, int x, int y, UNIVERSE_GRID **ugrid);
extern GRID_TYPE	Grid_Get(UNIVERSE *u, int x, int y, UNIVERSE_GRID *ugrid);
extern void		Grid_Clear(UNIVERSE *u, int x, int y);
extern void		Grid_SetBarrier(UNIVERSE *u, int x, int y);
extern void		Grid_SetOdor(UNIVERSE *u, int x, int y, KFORTH_INTEGER odor);
extern void		Grid_SetCell(UNIVERSE *u, CELL *cell);
extern void		Grid_SetOrganic(UNIVERSE *u, int x, int y, int energy);
extern void		Grid_SetSpore(UNIVERSE *u, int x, int y, SPORE *spore);

extern KFORTH_OPERATIONS *EvolveOperations(void);
extern void SimulationOptions_Init(SIMULATION_OPTIONS *so);
extern void StrainOptions_Init(STRAIN_OPTIONS *strain);

extern void		Universe_SetKey(UNIVERSE *u, int key);
extern void		Universe_ClearKey(UNIVERSE *u);
extern void		Universe_SetMouse(UNIVERSE *u, int x, int y);
extern void		Universe_ClearMouse(UNIVERSE *u);
extern void		Universe_update_protections(UNIVERSE *u, int strain, KFORTH_OPERATIONS* kfops, int protected_code_blocks);

/*
 * universe_creation.cpp
 */
extern void NewUniverseOptions_Init(NEW_UNIVERSE_OPTIONS *nuo);
extern STRAIN_PROFILE *NewUniverse_Get_StrainProfile(NEW_UNIVERSE_OPTIONS *nuo, int i);
extern UNIVERSE *CreateUniverse(NEW_UNIVERSE_OPTIONS *nuo, char *errbuf);

extern STRAIN_PROFILE* StrainProfile_Make();
extern void StrainProfile_Init(STRAIN_PROFILE *sp);
extern void StrainProfile_Set_Name(STRAIN_PROFILE *sp, const char *name);
extern void StrainProfile_Set_SeedFile(STRAIN_PROFILE *sp, const char *seed_file);
extern void StrainProfile_Set_Description(STRAIN_PROFILE *sp, const char *description);
extern const char *StrainProfile_Get_Description(STRAIN_PROFILE *sp);

extern void EvolvePreferences_Init(EVOLVE_PREFERENCES *ep);
extern void EvolvePreferences_Deinit(EVOLVE_PREFERENCES *ep);
extern EVOLVE_PREFERENCES* EvolvePreferences_Make();
extern void EvolvePreferences_Delete(EVOLVE_PREFERENCES* ep);
extern void EvolvePreferences_Add_StrainProfile(EVOLVE_PREFERENCES* ep, STRAIN_PROFILE *sp);
extern void EvolvePreferences_Clear_StrainProfiles(EVOLVE_PREFERENCES* ep);
extern STRAIN_PROFILE *EvolvePreferences_Get_StrainProfile(EVOLVE_PREFERENCES* ep, int i);
extern void EvolvePreferences_Create_From_Scratch(EVOLVE_PREFERENCES *ep);
extern int  EvolvePreferences_Load_Or_Create_From_Scratch(EVOLVE_PREFERENCES *ep, const char *filename, char *errbuf);
extern char* Evolve_Version();

/*
 * evolve_io.cpp
 */
extern UNIVERSE		*Universe_Read(const char *filename, char *errbuf);
extern int			Universe_Write(UNIVERSE *u, const char *filename, char *errbuf);

/*
 * evolve_io_ascii.cpp
 */
extern int			EvolvePreferences_Read(EVOLVE_PREFERENCES *ep, const char *filename, char *errbuf);
extern int			EvolvePreferences_Write(EVOLVE_PREFERENCES *ep, const char *filename, char *errbuf);

//
// Read and Write Ascii represention of the simulation using a external call back to read/write the data
//
extern intptr_t Universe_WriteAscii_CB(
						UNIVERSE *u,
						const char *name,
						intptr_t (*wcb)(const char *buf, intptr_t len),
						char *errbuf );

extern UNIVERSE *Universe_ReadAscii_CB(
						const char *name,
						intptr_t (*rcb)(char *buf, intptr_t reqlen),
						char *errbuf );

/*
 * Interface to the macos NSDocument object
 */
extern int EvolvePreferences_Read_CB(EVOLVE_PREFERENCES *ep,
						const char *name,
						intptr_t (*rcb)(char *buf, intptr_t reqlen),
						char *errbuf );

extern int EvolvePreferences_Write_CB( EVOLVE_PREFERENCES *ep,
						const char *name,
						intptr_t (*wcb)(const char *buf, intptr_t len),
						char *errbuf );

extern int Terrain_Read(UNIVERSE *u, const char *filename, char *errbuf);

/*
 * organism_finder.cpp
 */
typedef struct {
	int				error;
	char			error_message[1000];
	ORGANISM		*organism;
	int				min_energy;
	int				max_energy;
	int				avg_energy;
	int				min_age;
	int				max_age;
	int				avg_age;
	int				max_num_cells;
	int				reset_tracers;
	KFORTH_PROGRAM	*kfp;
	UNIVERSE		*u;
} ORGANISM_FINDER;

void 				OrganismFinder_init(ORGANISM_FINDER *of, const char *find_expr, int reset_tracers);
ORGANISM_FINDER*	OrganismFinder_make(const char *find_expr, int reset_tracers);
void				OrganismFinder_deinit(ORGANISM_FINDER *of);
void				OrganismFinder_delete(ORGANISM_FINDER *of);
void				OrganismFinder_execute(ORGANISM_FINDER *of, UNIVERSE *u);
const char *		OrganismFinder_get_error(ORGANISM_FINDER *of);

/*
 * random.cpp
 */
extern EVOLVE_RANDOM	*sim_random_make(uint32_t seed);
extern void				sim_random_init(uint32_t seed, EVOLVE_RANDOM *er);
extern void 			sim_random_delete(EVOLVE_RANDOM *er);
extern int32_t			sim_random(EVOLVE_RANDOM *er);

/*
 * kforth_interpreter.cpp
 */
typedef struct {
	EVOLVE_RANDOM	er;
	void			*p;
} KFORTH_INTERPRETER_CLIENT_DATA;

void KforthInterpreter_Replace_Instructions(KFORTH_OPERATIONS *kfops);

/*
 * cell_noops.cpp
 */
KFORTH_OPERATIONS *DummyEvolveOperations(void);

/*
 * swift_interface.cpp
 */
/*
 * Interface to the macos/swift NSDocument thing
 */
extern void Universe_Write_Using_CB(UNIVERSE *u);
extern UNIVERSE *Universe_Read_Using_CB();

extern KFORTH_INTEGER kforth_machine_ith_data_stack(KFORTH_MACHINE *kfm, int i);
extern KFORTH_INTEGER kforth_machine_ith_register(KFORTH_MACHINE *kfm, int i);
extern KFORTH_LOC kforth_machine_ith_call_stack(KFORTH_MACHINE *kfm, int i);
extern CELL_SENSE_DATA_ITEM CellSensoryData_ith_item(CELL_SENSE_DATA *csd, int dir);
extern KFORTH_INTEGER Universe_Get_Strain_Global(UNIVERSE *u, int strain);
extern KFORTH_MUTATE_OPTIONS*	Universe_get_ith_kfmo(UNIVERSE *u, int i);
extern KFORTH_OPERATIONS*		Universe_get_ith_kfops(UNIVERSE *u, int i);
extern STRAIN_OPTIONS*			Universe_get_ith_strop(UNIVERSE *u, int i);
extern void Universe_set_ith_kfmo(UNIVERSE *u, int i, KFORTH_MUTATE_OPTIONS* kfmo);
extern void Universe_set_ith_kfops(UNIVERSE *u, int i, KFORTH_OPERATIONS* kfops);
extern void Universe_set_ith_strop(UNIVERSE *u, int i, STRAIN_OPTIONS* strop);
extern void StrainOptions_Set_Name(STRAIN_OPTIONS *so, const char *name);
extern KFORTH_OPERATIONS* StrainProfile_get_kfops(STRAIN_PROFILE *sp);
extern EVOLVE_DFLT* EvolvePreferences_get_ith_dflt(EVOLVE_PREFERENCES* ep, int i);
extern void EvolvePreferences_set_ith_dflt_seed_file(EVOLVE_PREFERENCES* ep, int i, const char *seed_file);
extern void EvolvePreferences_set_terrain_file(EVOLVE_PREFERENCES* ep, const char *terrain_file);
extern void NewUniverse_Set_TerrainFile(NEW_UNIVERSE_OPTIONS *nuo, const char *terrain_file);

#ifdef __cplusplus
}
#endif

#endif
