//
//  Routines for creating a new universe
//

#include "evolve_simulator.h"
#include "evolve_simulator_private.h"

#define ROUND(x)	((int) (x))

static void create_barrier_point(UNIVERSE *u, int px, int py)
{
	int x, y;

	x = px;
	y = py;
	if( (x >= 0) && (x < u->width) && (y >= 0) && (y < u->height) )
		Universe_SetBarrier(u, x, y);

	x = px+1;
	y = py;
	if( (x >= 0) && (x < u->width) && (y >= 0) && (y < u->height) )
		Universe_SetBarrier(u, x, y);

	x = px-1;
	y = py;
	if( (x >= 0) && (x < u->width) && (y >= 0) && (y < u->height) )
		Universe_SetBarrier(u, x, y);

	x = px;
	y = py+1;
	if( (x >= 0) && (x < u->width) && (y >= 0) && (y < u->height) )
		Universe_SetBarrier(u, x, y);

	x = px;
	y = py-1;
	if( (x >= 0) && (x < u->width) && (y >= 0) && (y < u->height) )
		Universe_SetBarrier(u, x, y);
}

static void create_barrier_line(UNIVERSE *u, int p1x, int p1y, int p2x, int p2y)
{
	ASSERT( u != NULL );

	int x, y;
	double rise, run, m, b;
	int step;

	rise = (p2y - p1y);
	run = (p2x - p1x);

	if( run == 0.0 ) {
		// handle vertical line

		if( rise < 0.0 ) {
			step = -1;
		} else {
			step = 1;
		}

		y = p1y;
		while( y != p2y ) {
			create_barrier_point(u, p1x, y);
			y = y + step;
		}

	} else if( fabs(rise) <= fabs(run) ) {
			// iterate over x range
		m = rise / run;
		b = p1y - m * (p1x);

		if( run < 0 ) {
			step = -1;
		} else {
			step = 1;
		}

		x = p1x;
		while( x != p2x ) {
			y = ROUND(m*x + b);
			create_barrier_point(u, x, y);
			x = x + step;
		}

	} else {	// fabs(rise) > fabs(run)
			// iterate over y range
		m = rise / run;
		b = p1y - m * (p1x);

		if( rise < 0.0 ) {
			step = -1;
		} else {
			step = 1;
		}

		y = p1y;
		while( y != p2y ) {
			x = ROUND( (y-b) / m );
			create_barrier_point(u, x, y);
			y = y + step;
		}
	}
}


//
//	XCENTRE	X position of centre of ellipse in 
//		world coordinates.
//	YCENTRE	Same for Y position
//	MAJOR	Major axis of ellipse.
//	MINOR	Minor axis of ellipse.    
//	PA	Position angle of ellipse wrt. pos x-axis
//		in degrees.
//	STARTANG	Angle to start drawing ellipse (Degrees).
//	ENDANG	Angle to end drawing the ellipse (Degrees).
//	DELTA	Calculate plot points from STARTANG to ENDANG
//		in steps of DELTA degrees.
//
//Note:	The length of MAJOR can be smaller than the length of MINOR!
//
//Description:	Draw an ellipse with specifications (origin and axes) 
//		in world coordinates. The position angle of the major 
//		axis is wrt. the pos. x-axis. The ellipse is rotated 
//		counter-clockwise. The plotting starts at 'STARTANG'
//		degrees from the major axis and stops at 'ENDANG'
//		degrees from this major axis.
//
static void create_barrier_ellipse(UNIVERSE *u)
{
	double Pos_angle = 0.0;
	double Start_angle = 0.0;
	double End_angle = 360.0;
	double Rad = 0.017453292519943295769237;

	double delta;
	double center_x, center_y;	// center
	double major, minor;		// major axis and minor axis size

	double cosp, sinp;		// angles
	double cosa, sina;		// angles
	double alpha;			// used in polar coordinates
	double r;			// radius used in polar coords
	double denom;			// help variable
	double xell, yell;		// points of not rotated ellipse
	double x, y;			// each point of ellipse

	int curr_point_x, curr_point_y;
	int prev_point_x = 0, prev_point_y = 0;
	int first_point_x = 0, first_point_y = 0;

	bool first;
	double curve_angle;

	delta = 1.0;

	center_x = (u->width) / 2.0;
	center_y = (u->height) / 2.0;

	major = (u->width) / 2.0;
	minor = (u->height) / 2.0;

	cosp = cos( Pos_angle * Rad );
	sinp = sin( Pos_angle * Rad );

	first = true;
	alpha = Start_angle;
	curve_angle = 0.0;

	while( alpha <= End_angle ) {

		cosa = cos( alpha * Rad );
		sina = sin( alpha * Rad );
		denom = (minor*cosa * minor*cosa + major*sina * major*sina);
		if( denom == 0.0 ) {
			r = 0.0;
		} else {
			r = sqrt( minor*major * minor*major / denom );
		}

		xell = r * cosa;
		yell = r * sina;

		x = (xell * cosp - yell * sinp  + center_x);
		y = (xell * sinp + yell * cosp  + center_y);

		curr_point_x = ROUND(x);
		curr_point_y = ROUND(y);

		if( ! first ) {
			create_barrier_line(u, prev_point_x, prev_point_y, curr_point_x, curr_point_y);
		} else {
			first = false;
			first_point_x = curr_point_x;
			first_point_y = curr_point_y;
		}

		prev_point_x = curr_point_x;
		prev_point_y = curr_point_y;

		alpha = alpha + delta;

		curve_angle = curve_angle + 6;
		if( curve_angle > 360.0 ) {
			curve_angle = 0.0;
		}
	}

	create_barrier_line(u, prev_point_x, prev_point_y, first_point_x, first_point_y);
}

//
// the spiral path class. initialize with a starting
// point and a step value. Each time 'next' is called
// update the (x, y) point to be the next point in the spiral
// path.
//
//
typedef struct {
	int	x;
	int	y;
	int	step;

	int	dirx;
	int	diry;
	int	len;
	int	curlen;

} SPIRALPATH;

static SPIRALPATH *spiralpath_make(int x, int y, int step)
{
	SPIRALPATH *sp;

	sp = (SPIRALPATH *) CALLOC(1, sizeof(SPIRALPATH) );
	ASSERT( sp != NULL );

	sp->x		= x;
	sp->y		= y;
	sp->step	= step;

	sp->dirx	= 1;
	sp->diry	= 0;
	sp->len		= 1;

	sp->curlen	= sp->len;

	return sp;
}

/*
 * Compute next point in spiral path.
 */
static void spiralpath_next(SPIRALPATH *sp)
{
	int tmp;

	ASSERT( sp != NULL );

	if( sp->curlen > 0 ) {
		sp->curlen -= 1;
	} else {
		sp->len += 1;
		sp->curlen = sp->len;

		/*
		 * turn right
		 */
		tmp = sp->dirx;
		sp->dirx = sp->diry * -1;
		sp->diry = tmp;
	}

	sp->x += (sp->dirx * sp->step);
	sp->y += (sp->diry * sp->step);

}

static void spiralpath_delete(SPIRALPATH *sp)
{
	ASSERT( sp != NULL );
	FREE(sp);
}

/*
 * called this to create the
 * initial population.
 *
 * The organism 'o' will be cloned as needed. The organism 'o' will
 * be pasted as the first organism.
 *
 * total_energy is the amount of energy to divide up among the population.
 *
 *
 */
static void create_population(UNIVERSE *u, int population, int total_energy, ORGANISM *o)
{
	int energy_per_organism;
	int energy_remainder;
	int i;
	ORGANISM *no;
	SPIRALPATH *sp;

	ASSERT( u != NULL );
	ASSERT( population >= 1 && population <= 100 );
	ASSERT( total_energy > 0 );
	ASSERT( o != NULL );

	energy_per_organism = total_energy / population;
	energy_remainder = total_energy % population;

	if( energy_per_organism == 0 ) {
		population		= 1;
		energy_per_organism	= total_energy;
		energy_remainder	= 0;
	}

	sp = spiralpath_make(o->cells->x, o->cells->y, 5);

	for(i=0; i < population; i++) {

		if( i == 0 ) {
			no = o;
			no->energy = energy_per_organism + energy_remainder;
		} else {
			no = Universe_CopyOrganism(u);
			Universe_ClearSelectedOrganism(u);
			no->energy = energy_per_organism;
		}

		no->cells->x = sp->x;
		no->cells->y = sp->y;
		
		Universe_PasteOrganism(u, no);

		spiralpath_next(sp);
	}

	Universe_ClearSelectedOrganism(u);

	spiralpath_delete(sp);

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

/*
 * Setup the defaults
 *
 */
void NewUniverseOptions_Init(NEW_UNIVERSE_OPTIONS *nuo)
{
	int i;
	
//	seed = (long) GetTickCount();
	nuo->seed =  1234567; // KJS TODO Find "Mac" equivalent
	nuo->width = 700;
	nuo->height = 600;
	nuo->want_barrier = 1;
	nuo->terrain_file[0] = '\0';

	for(i=0; i<8; i++) {
		if( i == 0 ) {
			StrainProfile_Init( &nuo->strain_profiles[i] );
			nuo->strain_profiles[i].population = 1;
			nuo->strain_profiles[i].energy = 10000;
			kforth_mutate_options_defaults(&nuo->strain_profiles[i].kfmo);
		} else {
			StrainProfile_Init( &nuo->strain_profiles[i] );
			nuo->strain_profiles[i].population = 0;
			nuo->strain_profiles[i].energy = 0;
			kforth_mutate_options_defaults(&nuo->strain_profiles[i].kfmo);
		}
	}

}

STRAIN_PROFILE *NewUniverse_Get_StrainProfile(NEW_UNIVERSE_OPTIONS *nuo, int i)
{
	ASSERT( nuo != NULL );
	ASSERT( i >= 0 && i < 8 );

	return &nuo->strain_profiles[i];
}

/*
 * Create the universe from the properties that are
 * contained in the 'nuo' object.
 *
 * If only 1 strain was specified by the user, then
 * that starting organism goes right smack into the middle
 *
 * Otherwise, each strain is positioned around the place.
 *
 * 'errbuf' size assumed to be 1000.
 *
 */
UNIVERSE *CreateUniverse(NEW_UNIVERSE_OPTIONS *nuo, char *errbuf)
{
	int i, xpos, ypos;
	ORGANISM *o[8];
	UNIVERSE *u;
	int x[8], y[8];
	int wa, wb, wc;
	int ha, hb, hc;
	char buf[3000];
	char *source_code;
	FILE *fp;
	bool failed;
	int success;
	int posi;
	size_t len;
	int num_strains;
	STRAIN_PROFILE *sp;
	KFORTH_OPERATIONS *kfops;
	KFORTH_MUTATE_OPTIONS *kfmo;

	ASSERT( nuo != NULL );
	ASSERT( errbuf != NULL );

	num_strains = 0;
	for(i=0; i < 8; i++)
	{
		if( nuo->strain_profiles[i].strop.enabled == 1 ) {
			num_strains += 1;
		}
	}

	/*
 	 * Compute starting positions for each possible strain
	 */
	wa = nuo->width/4;
	wb = nuo->width/2;
	wc = nuo->width - nuo->width/4;

	ha = nuo->height/4;
	hb = nuo->height/2;
	hc = nuo->height - nuo->height/4;

	if( num_strains > 1 ) {
		x[0] = wb;
		y[0] = ha;

		x[1] = wb;
		y[1] = hc;

		x[2] = wa;
		y[2] = hb;

		x[3] = wc;
		y[3] = hb;

		x[4] = wa;
		y[4] = ha;

		x[5] = wc;
		y[5] = ha;

		x[6] = wa;
		y[6] = hc;

		x[7] = wc;
		y[7] = hc;
	} else {
		/*
		 * If just 1 strain in sim, then put it in the middle.
		 */
		for(i=0; i<8; i++) {
			x[i] = wb;
			y[i] = hb;
		}
	}

	for(i=0; i<8; i++) {
		o[i] = NULL;
	}

	posi = 0;
	failed = false;
	for(i=0; i<8; i++) {
		sp = &nuo->strain_profiles[i];
		
		if( ! sp->strop.enabled ) {
			continue;
		}

		xpos = x[posi];
		ypos = y[posi];

		fp = fopen(sp->seed_file, "r");
		if( fp == NULL ) {
			snprintf(errbuf, 1000, "Strain %d, %s: %s", i, sp->seed_file, strerror(errno));
			failed = true;
			break;
		}

		size_t sclen, newlen;

		sclen = 0;
		source_code = STRDUP("");
		while( fgets(buf, sizeof(buf), fp) != NULL ) {
			len = strlen(buf)-1;
			if( buf[len-1] == '\n' )
				buf[ strlen(buf)-1 ] = '\0';
			strcat(buf, "\r\n");
			len = strlen(buf);

			newlen = sclen + len + 1;

			source_code = (char*)realloc(source_code, newlen);
			strcat(source_code, buf);
			sclen = newlen;
		}
		fclose(fp);

		kfops = &nuo->strain_profiles[i].kfops;
		kfmo = &nuo->strain_profiles[i].kfmo;
		o[i] = Organism_Make(
						xpos, ypos,
						i, sp->energy,
						kfops,
						kfmo->protected_codeblocks,
						source_code,
						buf );

		if( o[i] == NULL ) {
			snprintf(errbuf, 1000, "Strain %d, %s: %s", i, sp->seed_file, buf);
			failed = true;
			free(source_code);
			break;
		}

		free(source_code);

		posi++;
	}

	if( failed ) {
		for(i=0; i<8; i++) {
			if( o[i] == NULL ) {
				continue;
			}
			Organism_delete(o[i]);
		}
		return NULL;
	}

	u = Universe_Make(nuo->seed, nuo->width, nuo->height);
	ASSERT( u != NULL );

	if( nuo->want_barrier ) {
		create_barrier_ellipse(u);
	}

	for(i=0; i<8; i++) {
		if( o[i] == NULL )
			continue;

		sp = &nuo->strain_profiles[i];

		u->strop[i] = sp->strop;
		u->kfmo[i] = sp->kfmo;
		u->kfops[i] = sp->kfops;

		create_population(u, sp->population, sp->energy, o[i] );
	}

	u->so = nuo->so;

	//
	// ensure strain 0 always exists
	//
	if( num_strains == 0 )
	{
		u->strop[0].enabled = 1;
		kforth_mutate_options_defaults(&u->kfmo[0]);
		u->kfops[0] = *EvolveOperations();
	}

	if( strlen(nuo->terrain_file) > 0 ) {
		success = Terrain_Read(u, nuo->terrain_file, errbuf);
		if( !success ) {
			Universe_Delete(u);
			return NULL;
		}
	}

	return u;
}

/* **********************************************************************
 *
 * Evolve Preferences and Stain Profiles
 *
 * Object Make, Delete, Deinit, Init
 *
 */

STRAIN_PROFILE* StrainProfile_Make()
{
	STRAIN_PROFILE *sp;

	sp = (STRAIN_PROFILE*) CALLOC(1, sizeof(STRAIN_PROFILE) );
	ASSERT( sp != NULL );

	return sp;
}

void StrainProfile_Init(STRAIN_PROFILE *sp)
{
	memset(sp, 0, sizeof(STRAIN_PROFILE));
}

void StrainProfile_Deinit(STRAIN_PROFILE *sp)
{
	// nothing to do
}

void StrainProfile_Set_Name(STRAIN_PROFILE *sp, const char *name)
{
	strlcpy(sp->name, name, sizeof(sp->name));
}

void StrainProfile_Set_SeedFile(STRAIN_PROFILE *sp, const char *seed_file)
{
	strlcpy(sp->seed_file, seed_file, sizeof(sp->seed_file));
}

void StrainProfile_Set_Description(STRAIN_PROFILE *sp, const char *description)
{
	strlcpy(sp->description, description, sizeof(sp->description));
}

const char *StrainProfile_Get_Description(STRAIN_PROFILE *sp)
{
	return sp->description;
}

void EvolvePreferences_Init(EVOLVE_PREFERENCES *ep)
{
	ASSERT( ep != NULL );

	memset(ep, 0, sizeof(EVOLVE_PREFERENCES));
}

void EvolvePreferences_Deinit(EVOLVE_PREFERENCES *ep)
{
	ASSERT( ep != NULL );

	FREE(ep->strain_profiles);
}

EVOLVE_PREFERENCES* EvolvePreferences_Make()
{
	EVOLVE_PREFERENCES *ep;

	ep = (EVOLVE_PREFERENCES*) CALLOC(1, sizeof(EVOLVE_PREFERENCES) );
	ASSERT( ep != NULL );

	EvolvePreferences_Init(ep);

	return ep;
}

void EvolvePreferences_Delete(EVOLVE_PREFERENCES* ep)
{
	EvolvePreferences_Deinit(ep);
	FREE(ep);
}

//
// Increase allocation for ep->strain_profiles to hold 1 more, and add 'sp' to end.
//
void EvolvePreferences_Add_StrainProfile(EVOLVE_PREFERENCES* ep, STRAIN_PROFILE *sp)
{
	int i;

	ASSERT( ep != NULL );
	ASSERT( sp != NULL );

	ep->strain_profiles = (STRAIN_PROFILE*) REALLOC(ep->strain_profiles, sizeof(STRAIN_PROFILE) * (ep->nprofiles+1) );
	ASSERT( ep->strain_profiles != NULL );

	i = ep->nprofiles;
	ep->nprofiles += 1;

	ep->strain_profiles[i] = *sp;
}

void EvolvePreferences_Clear_StrainProfiles(EVOLVE_PREFERENCES* ep)
{
	int i;

	for(i=0; i < ep->nprofiles; i++) {
		StrainProfile_Deinit(&ep->strain_profiles[i]);
	}

	FREE(ep->strain_profiles);
	ep->strain_profiles = NULL; // needed so realloc() works
	ep->nprofiles = 0;
}

extern STRAIN_PROFILE *EvolvePreferences_Get_StrainProfile(EVOLVE_PREFERENCES* ep, int i)
{
	ASSERT( ep != NULL );
	ASSERT( i >= 0 && i < ep->nprofiles );

	return &ep->strain_profiles[i];
}

/***********************************************************************
 * Create the first Evolve_Preferences for the application when
 * there was nothing to read from an existing ~/.evolve5rc file
 *
 */
void EvolvePreferences_Create_From_Scratch(EVOLVE_PREFERENCES *ep)
{
	int i;

	ASSERT( ep != NULL );

	EvolvePreferences_Init(ep);

	strlcat(ep->evolve_batch_path, "/tmp/EvolveBatch.app", sizeof(ep->evolve_batch_path));
	strlcat(ep->evolve_3d_path, "/tmp/Evolve3d.app", sizeof(ep->evolve_3d_path));
	strlcat(ep->help_path, "", sizeof(ep->help_path));

	ep->width			= 600;
	ep->height			= 400;
	ep->want_barrier	= 1;

	ep->dflt[0].profile_idx	= 0;
	ep->dflt[0].energy			= 1000000;
	ep->dflt[0].population		= 100;
	strlcat(ep->dflt[0].seed_file, "", sizeof(ep->dflt[0].seed_file));

	for(i=1; i < 8; i++) {
		ep->dflt[i].profile_idx = -1;
	}

	//////////////////////////
	//
	// add strain profile 0
	//
	STRAIN_PROFILE sp;

	StrainProfile_Init(&sp);

	strlcat(sp.name, 				"Default", sizeof(sp.name));
	strlcat(sp.seed_file,			"/tmp/default.kf", sizeof(sp.seed_file));
	strlcat(sp.description,			"Default. A Creature\nThat\nJust Works.", sizeof(sp.description));
	sp.energy						= 100000;
	sp.population					= 10;
	kforth_mutate_options_defaults(&sp.kfmo);
	sp.kfops						= *EvolveOperations();

	kforth_ops_set_protected(&sp.kfops, "SPAWN");
	kforth_ops_set_protected(&sp.kfops, "MAKE-BARRIER");

	sp.strop.enabled				= 1;	// indicates if this strain is enabled or not
	sp.strop.name[0]				= 0;	// strain name, not meaningful here
	sp.strop.look_mode				= 1;	// LOOK: 0=default/original. 1=cannot see thru self.
	sp.strop.eat_mode				= 0;	// EAT: 0=default/original. 1=can eat self, killing self.
	sp.strop.make_spore_mode		= 0;	// MAKE-SPORE mode
	sp.strop.make_spore_energy		= 100;	// MAKE-SPORE energy
	sp.strop.cmove_mode				= 0;	// CMOVE: 0=default.
	sp.strop.omove_mode				= 0;	// OMOVE 0=default.
	sp.strop.grow_mode				= 0;	// GROW mode
	sp.strop.grow_energy			= 10;	// GROW energy
	sp.strop.grow_size				= 20;	// GROW size
	sp.strop.rotate_mode			= 1;	// ROTATE: 0=default 90 deg. 1=45 deg (if diagonal) must use center point of orgnism
	sp.strop.cshift_mode			= 0;	// CSHIFT: 0=default.
	sp.strop.make_organic_mode		= 0;	// MAKE-ORGANIC: 0=default

	EvolvePreferences_Add_StrainProfile(ep, &sp);

	StrainProfile_Init(&sp);
	strlcat(sp.name, 				"Evolve4.8e", sizeof(sp.name));
	strlcat(sp.seed_file,			"/tmp/seed48.kf", sizeof(sp.seed_file));
	strlcat(sp.description,			"Original Evolve4.8e behavior", sizeof(sp.description));
	sp.energy						= 100000;
	sp.population					= 10;
	kforth_mutate_options_defaults(&sp.kfmo);
	sp.kfops						= *EvolveOperations();

	sp.strop.enabled				= 1;	// indicates if this strain is enabled or not
	sp.strop.name[0]				= 0;	// strain name
	sp.strop.look_mode				= 0;	// LOOK: 0=default/original. 1=cannot see thru self.
	sp.strop.eat_mode				= 0;	// EAT: 0=default/original. 1=can eat self, killing self.
	sp.strop.make_spore_mode		= 0;	// MAKE-SPORE: 0=default/original. sexual only. asexual only.
	sp.strop.make_spore_energy		= 100;	// MAKE-SPORE energy
	sp.strop.cmove_mode				= 0;	// CMOVE: 0=default.
	sp.strop.omove_mode				= 0;	// OMOVE 0=default.
	sp.strop.grow_mode				= 0;	// GROW: 0=default, 1=can grow to an occupied square shifting self cells
	sp.strop.grow_energy			= 10;	// GROW energy
	sp.strop.grow_size				= 50;	// GROW size
	sp.strop.rotate_mode			= 0;	// ROTATE: 0=default 90 deg. 1=45 deg (if diagonal) must use center point of orgnism
	sp.strop.cshift_mode			= 0;	// CSHIFT: 0=default.
	sp.strop.make_organic_mode		= 0;	// MAKE-ORGANIC: 0=default

	EvolvePreferences_Add_StrainProfile(ep, &sp);

	StrainProfile_Init(&sp);

	strlcat(sp.name, 				"Basic", sizeof(sp.name));
	strlcat(sp.seed_file,			"/tmp/basic.kf", sizeof(sp.seed_file));
	strlcat(sp.description,			"Basic Creature.", sizeof(sp.description));
	sp.energy						= 100000;
	sp.population					= 1;
	kforth_mutate_options_defaults(&sp.kfmo);
	sp.kfops						= *EvolveOperations();

	sp.strop.enabled				= 1;	// indicates if this strain is enabled or not
	sp.strop.name[0]				= 0;	// strain name, not meaningful here
	sp.strop.look_mode				= 1;	// LOOK: 0=default/original. 1=cannot see thru self.
	sp.strop.eat_mode				= 1;	// EAT: 0=default/original. 1=can eat self, killing self.
	sp.strop.make_spore_mode		= 0;	// MAKE-SPORE: 0=default/original. sexual only. asexual only.
	sp.strop.make_spore_energy		= 100;	// MAKE-SPORE energy
	sp.strop.cmove_mode				= 0;	// CMOVE: 0=default.
	sp.strop.omove_mode				= 0;	// OMOVE 0=default.
	sp.strop.grow_mode				= 0;	// GROW mode
	sp.strop.grow_energy			= 10;	// GROW energy
	sp.strop.grow_size				= 10;	// GROW size
	sp.strop.rotate_mode			= 1;	// ROTATE: 0=default 90 deg. 1=45 deg (if diagonal) must use center point of orgnism
	sp.strop.cshift_mode			= 0;	// CSHIFT: 0=default.
	sp.strop.make_organic_mode		= 0;	// MAKE-ORGANIC: 0=default

	EvolvePreferences_Add_StrainProfile(ep, &sp);
}

/*
 * If 'filename' exists, populate 'ep' with those evolve preferences.
 * Else populate 'ep' with internal appication "first time" defaults.
 *
 */
int EvolvePreferences_Load_Or_Create_From_Scratch(EVOLVE_PREFERENCES *ep, const char *filename, char *errbuf)
{
	FILE *fp;

	// check if file exists
	fp = fopen(filename, "r");
	if( fp == NULL )
	{
		EvolvePreferences_Create_From_Scratch(ep);
		return 1;
	}

	fclose(fp);

	return EvolvePreferences_Read(ep, filename, errbuf);
}

char *Evolve_Version()
{
	static int first_time = 1;
	static int evolve_major_number = 5;
	static int evolve_minor_number = 0;
	static char version_string[100];
	char modes[100];

	if( first_time )
	{
		first_time = 0;

		strcpy(modes, " ");

#ifdef EVOLVE_DEBUG
		strcat(modes, "DEBUG ");
#else
		strcat(modes, "RELEASE ");
#endif

		snprintf(version_string, sizeof(version_string), " Evolve v%d.%d  %s  %s  (%s)",
			evolve_major_number,
			evolve_minor_number,
			__DATE__,
			__TIME__,
			modes);
	}

	return version_string;
}
