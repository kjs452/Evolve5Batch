#!/usr/bin/python3
#
#
#
import sys
import math
import random

W = 600
H = 400

background=(100,100,100)
gradtable = [ (0,0) for i in range(0,W*H) ]

def precalc_gradtable():
    rnd = random.Random()
    for i in range(0,H):
        for j in range(0, W):
            x = float((rnd.randint(1,2*W))-W)/W
            y = float((rnd.randint(1,2*H))-H)/H
            s = math.sqrt(x * x + y * y)
            if s!=0:
                x = x / s
                y = y / s
            else:
                x = 0
                y = 0
            gradtable[i*H+j] = (x,y)

#calculate dot product for v1 and v2
def dot(v1,v2):
    return ( (v1[0]*v2[0]) + (v1[1]*v2[1]) )

# get a pseudorandom gradient vector
def gradient(x,y):
    # normalize!
    return gradtable[y*H+x]

def s_curve(x):
    return ( 3*x*x - 2*x*x*x )

def noise2d(x,y):
    x0 = math.floor(x)
    y0 = math.floor(y)
    x1 = x0 + 1.0
    y1 = y0 + 1.0

    i_x0 = int(x0)
    i_x1 = int(x1)
    i_y0 = int(y0)
    i_y1 = int(y1)

    s = dot(gradient(i_x0, i_y0),(x-x0, y-y0))
    t = dot(gradient(i_x1, i_y0),(x-x1, y-y0))
    u = dot(gradient(i_x0, i_y1),(x-x0, y-y1))
    v = dot(gradient(i_x1, i_y1),(x-x1, y-y1))

    s_x = s_curve( x - x0 )
    a = s + s_x*t - s_x*s
    b = u + s_x*v - s_x*u

    s_y = s_curve( y - y0 )
    z = a + s_y*b - s_y*a

    return z

def col( a ):
    return int(round((128-(128*a))))

#### precalc_gradtable()
####
#### img = Image.new("RGB", (W,H), background)
#### draw = ImageDraw.Draw(img)
#### 
#### zoom_x = 0.04
#### zoom_y = 0.04
#### x = 0.0
#### y = 0.0
#### for j in range(0,H):
####     for i in range(0,W):
####         a = col(noise2d(x,y))
####         draw.point((i,j),fill=(a,a,a))
####         x = x + zoom_x
####     y = y + zoom_y
####     x = 0.0
#### img.save("/tmp/perlin2d.png","png")
#### 

def NOISE(x, y):
	z = noise2d(x, y)
#	sys.stdout.write("noise2d %f %f %f col=%d\n" % (x, y, z, col(z)))
	return col(z) > 160

def prolog(width, height):
	sys.stdout.write("""# PHOTON ASCII
#
# this file contains a dummy universe, enough to define a
# set of barriers for import
# by the New Universe Dialog
#

struct UNIVERSE {
	SEED
	STEP
	AGE
	CURRENT_CELL { X Y }    # -1 -1 means NULL
	NEXT_ID
	NBORN
	NDIE
	WIDTH
	HEIGHT
	G0
	KEY
	MOUSE_X
	MOUSE_Y
	S0[N] { V }
}

struct BARRIER[N] {
	X
	Y
}

UNIVERSE 0          # seed
         0          # step
         0          # age
         -1 -1      # current cell location (x,y)
         0          # next id
         0 0      	# number births, deaths
         %d %d    # dimensions: width x height
         0          # global register G0
         0          # key
         -1         # mouse_x
         -1         # mouse_y
       { 0 0 0 0 0 0 0 0 }  # S0's for each strain

""" % (width, height))

def main():
	BATCH_SIZE = 1000

	precalc_gradtable()

	prolog(W, H)

	count = 0
	zoom_x = 0.04
	zoom_y = 0.04
	x = 0.0
	y = 0.0
	for j in range(0, H):
		for i in range(0, W):

			on = NOISE(x, y)
			if on:
				if count % BATCH_SIZE == 0:
					sys.stdout.write("BARRIER {\n")

				sys.stdout.write("	%d	%d\n" % (i, j))
				count += 1

				if count % BATCH_SIZE == 0:
					sys.stdout.write("}\n\n")

			x = x + zoom_x
		y = y + zoom_y
		x = 0.0

	if count % BATCH_SIZE != 0:
		sys.stdout.write("}\n")

main()
