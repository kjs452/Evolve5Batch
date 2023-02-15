//
// This module implements the logic for the 'Find' feature.
//
// It will evaluate a find expression and set the radioactive tracer
// flag for organisms that match the expression.
//
// The find expression is given in KFORTH notation with special
// find instructions.
//
// To use:
//	1. Create instance of this class, give the find_expression
//	2. Check if error is TRUE or FALSE.
//	3. if error is FALSE, then error_message will explain the problem
//	4. Otherwise call execute() with the simulation to use.
//	5. Now all organisms that match the expression will have a radioactive tracer.
//
// 'reset_tracers' will first clear any previous tracers.
//
// evaluates a find_expression for all organisms
// and sets their raddioactive tracer when the expression evaluates to true.
//
#include "evolve_simulator.h"
#include "evolve_simulator_private.h"

/*
 * when a value to be returned back to the KFORTH_MACHINE is too big, it
 * exceeds this value. This is max_int.
 */
#define TOO_BIG		32767

static KFORTH_OPERATIONS *FindOperations(void);

void OrganismFinder_init(ORGANISM_FINDER *of, const char *find_expr, int reset_tracers)
{
	char buf[1000];

	ASSERT( find_expr != NULL );

	of->reset_tracers = reset_tracers;

	if( strchr(find_expr, '{') == NULL ) {
		snprintf(buf, sizeof(buf), "{ %s }", find_expr);
	} else {
		snprintf(buf, sizeof(buf), "%s", find_expr);
	}

	of->kfp = kforth_compile(buf, FindOperations(), of->error_message);
	if( of->kfp == NULL ) {
		of->error = 1;
	} else {
		of->error = 0;
		strcpy(of->error_message, "");
	}
}

ORGANISM_FINDER *OrganismFinder_make(const char *find_expr, int reset_tracers)
{
	ORGANISM_FINDER* of;

	ASSERT( find_expr != NULL );

	of = (ORGANISM_FINDER*) CALLOC(1, sizeof(ORGANISM_FINDER));
	ASSERT( of != NULL );

	OrganismFinder_init(of, find_expr, reset_tracers);

	return of;
}

void OrganismFinder_deinit(ORGANISM_FINDER *of)
{
	ASSERT( of != NULL );

	kforth_delete(of->kfp);
}

void OrganismFinder_delete(ORGANISM_FINDER *of)
{
	OrganismFinder_deinit(of);
	FREE(of);
}

const char *OrganismFinder_get_error(ORGANISM_FINDER *of)
{
	return of->error_message;
}

//
// execute KFORTH program. If top of stack is non-zero then
// return true, else return false.
//
static int evalute(ORGANISM_FINDER *of, KFORTH_MACHINE *kfm, ORGANISM *o)
{
	KFORTH_INTEGER val;
	int n;

	ASSERT( kfm != NULL );
	ASSERT( o != NULL );

	of->organism = o;
	kforth_machine_reset(kfm);

	//
	// Run machine until terminated, or number of
	// steps exceeds 1,000.
	//
	for(n=0; n < 1000; n++) {
		kforth_machine_execute(FindOperations(), of->kfp, kfm, of);
		if( kforth_machine_terminated(kfm) ) {
			break;
		}
	}

	if( kforth_machine_terminated(kfm) ) {
		if( kfm->dsp == 1 ) {
			val = kforth_data_stack_pop(kfm);
			if( val == 0 ) {
				// non-match
				return 0;
			} else {
				// match
				return 1;
			}
		} else {
			//
			// expression terminated but data stack is
			// something other than 1 in length, we
			// will treat this case as false.
			//
			return 0;
		}
	} else {
		//
		// somehow the damn expression never finished within
		// 1000 steps without terminating. Therefore, we
		// just assume a bug and return false.
		// A non-looping, non-recursive
		// expression should always terminate.
		//
		return 0;
	}
}

void OrganismFinder_execute(ORGANISM_FINDER *of, UNIVERSE *u)
{
	ORGANISM *o;
	KFORTH_MACHINE *kfm;
	KFORTH_INTEGER sum_energy, sum_age;

	ASSERT( of != NULL );
	ASSERT( u != NULL );
	ASSERT( ! of->error );

	of->u = u;

	if( of->reset_tracers ) {
		Universe_ClearTracers(u);
	}

	//
	// Examine all organisms and compute
	// the min/max/avg values
	//
	sum_energy			= 0;
	sum_age				= 0;

	of->min_energy		= 999999;
	of->max_energy		= -1;

	of->min_age			= 999999;
	of->max_age			= -1;

	of->max_num_cells	= -1;

	for(o=u->organisms; o != NULL; o=o->next) {
		//
		// Find min/max/avg constants
		//
		sum_energy += o->energy;
		sum_age += o->age;

		if( o->energy < of->min_energy )
			of->min_energy = o->energy;

		if( o->energy > of->max_energy )
			of->max_energy = o->energy;

		if( o->age < of->min_age )
			of->min_age = o->age;

		if( o->age > of->max_age )
			of->max_age = o->age;

		if( o->ncells > of->max_num_cells )
			of->max_num_cells = o->ncells;

	}

	if( u->norganism > 0 ) {
		of->avg_energy	= (int)(sum_energy / u->norganism);
		of->avg_age		= (int)(sum_age / u->norganism);
	} else {
		of->avg_energy	= 0;
		of->avg_age		= 0;
	}

	kfm = kforth_machine_make();
	ASSERT( kfm != NULL );

	for(o=u->organisms; o != NULL; o=o->next) {
		if( evalute(of, kfm, o) ) {
			o->oflags |= ORGANISM_FLAG_RADIOACTIVE;
		}
	}

	kforth_machine_delete(kfm);
}

// last 4 digits of the id...
//		32000
//		 9999
//		 1122  heh heh heh
static void FindOpcode_ID(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	ORGANISM *o;
	char buf[50];
	KFORTH_INTEGER val;

	ofc = (ORGANISM_FINDER*) client_data;
	o = ofc->organism;

	snprintf(buf, sizeof(buf), "%lld", o->id);
	val = atoi(buf + strlen(buf)-4 );

	kforth_data_stack_push(kfm, val);
}

// last 4 digits:	9999
static void FindOpcode_PARENT1(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	ORGANISM *o;
	char buf[50];
	KFORTH_INTEGER val;

	ofc = (ORGANISM_FINDER*) client_data;
	o = ofc->organism;

	snprintf(buf, sizeof(buf), "%lld", o->parent1);
	val = atoi(buf + strlen(buf)-4 );

	kforth_data_stack_push(kfm, val);
}

// last 4 digits:	9999
static void FindOpcode_PARENT2(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	ORGANISM *o;
	char buf[50];
	KFORTH_INTEGER val;

	ofc = (ORGANISM_FINDER*) client_data;
	o = ofc->organism;

	snprintf(buf, sizeof(buf), "%lld", o->parent2);
	val = atoi(buf + strlen(buf)-4 );

	kforth_data_stack_push(kfm, val);
}

static void FindOpcode_STRAIN(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	ORGANISM *o;

	ofc = (ORGANISM_FINDER*) client_data;
	o = ofc->organism;

	kforth_data_stack_push(kfm, o->strain);
}

static void FindOpcode_ENERGY(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	ORGANISM *o;
	KFORTH_INTEGER val;

	ofc = (ORGANISM_FINDER*) client_data;
	o = ofc->organism;
	val = (o->energy < TOO_BIG) ? o->energy : TOO_BIG;
	kforth_data_stack_push(kfm, val);
}

static void FindOpcode_GENERATION(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	ORGANISM *o;
	KFORTH_INTEGER val;

	ofc = (ORGANISM_FINDER*) client_data;
	o = ofc->organism;
	val = (o->generation < TOO_BIG) ? o->generation : TOO_BIG;
	kforth_data_stack_push(kfm, val);
}

static void FindOpcode_NUM_CELLS(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	ORGANISM *o;
	KFORTH_INTEGER val;

	ofc = (ORGANISM_FINDER*) client_data;
	o = ofc->organism;
	val = (o->ncells < TOO_BIG) ? o->ncells : TOO_BIG;
	kforth_data_stack_push(kfm, val);
}

// Scaled to thousands: 1 AGE >=
static void FindOpcode_AGE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	ORGANISM *o;
	KFORTH_INTEGER val;
	LONG_LONG scaled_age;

	ofc = (ORGANISM_FINDER*) client_data;
	o = ofc->organism;

	scaled_age = o->age / 1000;
	val = (scaled_age < TOO_BIG) ? scaled_age : TOO_BIG;
	kforth_data_stack_push(kfm, val);
}

static void FindOpcode_NCHILDREN(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	ORGANISM *organism, *o;
	UNIVERSE *u;
	KFORTH_INTEGER val;
	int num_living_children;

	ofc = (ORGANISM_FINDER*) client_data;
	organism = ofc->organism;
	u = ofc->u;

	num_living_children = 0;
	for(o = u->organisms; o; o=o->next) {
		if( o->parent1 == organism->id || o->parent2 == organism->id ) {
			num_living_children += 1;
		}
	}

	val = (num_living_children < TOO_BIG) ? num_living_children : TOO_BIG;

	kforth_data_stack_push(kfm, num_living_children);
}

static void FindOpcode_EXECUTING(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	ORGANISM *o;
	CELL *c;
	KFORTH_INTEGER val;
	int found;

	ofc = (ORGANISM_FINDER*) client_data;
	o = ofc->organism;

	val = kforth_data_stack_pop(kfm);

	found = 0;
	for(c=o->cells; c != NULL; c=c->next) {
		if( c->kfm.loc.cb == val ) {
			found = 1;
			break;
		}
	}

	if( found ) {
		kforth_data_stack_push(kfm, 1);
	} else {
		kforth_data_stack_push(kfm, 0);
	}
}

static void FindOpcode_NUM_CB(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	ORGANISM *o;
	KFORTH_PROGRAM *okfp;

	ofc = (ORGANISM_FINDER*) client_data;
	o = ofc->organism;
	okfp = &o->program;

	kforth_data_stack_push(kfm, okfp->nblocks);
}

static void FindOpcode_NUM_DEAD(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	ORGANISM *o;
	CELL *c;
	KFORTH_INTEGER num_dead;

	ofc = (ORGANISM_FINDER*) client_data;
	o = ofc->organism;

	num_dead = 0;
	for(c=o->cells; c != NULL; c=c->next) {
		if( kforth_machine_terminated(&c->kfm) ) {
			num_dead += 1;
		}
	}

	kforth_data_stack_push(kfm, num_dead);
}

static void FindOpcode_MAX_ENERGY(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	int energy;
	KFORTH_INTEGER val;

	ofc = (ORGANISM_FINDER*) client_data;
	energy = ofc->max_energy;
	val = (energy < TOO_BIG) ? energy : TOO_BIG;
	kforth_data_stack_push(kfm, val);
}

static void FindOpcode_MIN_ENERGY(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	int energy;
	KFORTH_INTEGER val;

	ofc = (ORGANISM_FINDER*) client_data;
	energy = ofc->min_energy;
	val = (energy < TOO_BIG) ? energy : TOO_BIG;
	kforth_data_stack_push(kfm, val);
}

static void FindOpcode_AVG_ENERGY(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	int energy;
	KFORTH_INTEGER val;

	ofc = (ORGANISM_FINDER*) client_data;
	energy = ofc->avg_energy;
	val = (energy < TOO_BIG) ? energy : TOO_BIG;
	kforth_data_stack_push(kfm, val);
}

// scaled 1 : 1000 age
static void FindOpcode_MAX_AGE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	LONG_LONG scaled_age;
	KFORTH_INTEGER val;

	ofc = (ORGANISM_FINDER*) client_data;
	scaled_age = ofc->max_age / 1000;
	val = (scaled_age < TOO_BIG) ? scaled_age : TOO_BIG;
	kforth_data_stack_push(kfm, val);
}

// scaled 1 : 1000 age
static void FindOpcode_MIN_AGE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	LONG_LONG scaled_age;
	KFORTH_INTEGER val;

	ofc = (ORGANISM_FINDER*) client_data;
	scaled_age = ofc->min_age / 1000;
	val = (scaled_age < TOO_BIG) ? scaled_age : TOO_BIG;
	kforth_data_stack_push(kfm, val);
}

// scaled 1 : 1000 age
static void FindOpcode_AVG_AGE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	LONG_LONG scaled_age;
	KFORTH_INTEGER val;

	ofc = (ORGANISM_FINDER*) client_data;
	scaled_age = ofc->avg_age / 1000;
	val = (scaled_age < TOO_BIG) ? scaled_age : TOO_BIG;
	kforth_data_stack_push(kfm, val);
}

static void FindOpcode_MAX_NUM_CELLS(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM_FINDER *ofc;
	KFORTH_INTEGER val;

	ofc = (ORGANISM_FINDER*) client_data;
	val = (ofc->max_num_cells < TOO_BIG) ? ofc->max_num_cells : TOO_BIG;
	kforth_data_stack_push(kfm, val);
}

//
// A "once" function, always returns pointer to same static table
// for all instances and all callers.
//
// Returns a table of KFORTH operations for the FIND feature.
//
static KFORTH_OPERATIONS *FindOperations(void)
{
	static int first_time = 1;
	static KFORTH_OPERATIONS kfops;

	if( first_time ) {
		first_time = 0;
		kforth_ops_init(&kfops);

		kforth_ops_add(&kfops,	"ID",				0, 1, FindOpcode_ID);
		kforth_ops_add(&kfops,	"PARENT1",			0, 1, FindOpcode_PARENT1);
		kforth_ops_add(&kfops,	"PARENT2",			0, 1, FindOpcode_PARENT2);
		kforth_ops_add(&kfops,	"STRAIN",			0, 1, FindOpcode_STRAIN);
		kforth_ops_add(&kfops,	"ENERGY",			0, 1, FindOpcode_ENERGY);
		kforth_ops_add(&kfops,	"GENERATION",		0, 1, FindOpcode_GENERATION);
		kforth_ops_add(&kfops,	"NUM-CELLS",		0, 1, FindOpcode_NUM_CELLS);
		kforth_ops_add(&kfops,	"AGE",				0, 1, FindOpcode_AGE);
		kforth_ops_add(&kfops,	"NCHILDREN",		0, 1, FindOpcode_NCHILDREN);
		kforth_ops_add(&kfops,	"EXECUTING",		1, 1, FindOpcode_EXECUTING);
		kforth_ops_add(&kfops,	"NUM-CB",			0, 1, FindOpcode_NUM_CB);
		kforth_ops_add(&kfops,	"NUM-DEAD",			0, 1, FindOpcode_NUM_DEAD);
		kforth_ops_add(&kfops,	"MAX-ENERGY",		0, 1, FindOpcode_MAX_ENERGY);
		kforth_ops_add(&kfops,	"MIN-ENERGY",		0, 1, FindOpcode_MIN_ENERGY);
		kforth_ops_add(&kfops,	"AVG-ENERGY",		0, 1, FindOpcode_AVG_ENERGY);
		kforth_ops_add(&kfops,	"MAX-AGE",			0, 1, FindOpcode_MAX_AGE);
		kforth_ops_add(&kfops,	"MIN-AGE",			0, 1, FindOpcode_MIN_AGE);
		kforth_ops_add(&kfops,	"AVG-AGE",			0, 1, FindOpcode_AVG_AGE);
		kforth_ops_add(&kfops,	"MAX-NUM-CELLS",	0, 1, FindOpcode_MAX_NUM_CELLS);
	}

	return &kfops;
}
