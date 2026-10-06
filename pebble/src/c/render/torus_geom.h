/* Torus geometry, camera and projection. Pure integer maths, no Pebble APIs.
 *
 * Board geometry : (row,col) -> surface patch (V interval, U interval).
 *                  U = around the main ring, V = around the tube. col<->U, row<->V.
 * Camera         : spin (about the torus axis) + pitch (tilt of the axis). Never touches game state.
 * Projection     : orthographic, scaled to whatever pixel region it is given.
 *
 * Angles: unsigned 16-bit turn fraction (65536 = 360 deg). Model coords Q12 (4096 = 1.0).
 * Matrices Q14 (16384 = 1.0). */
#ifndef TORUS_GEOM_H
#define TORUS_GEOM_H
#include <stdint.h>
#include <stdbool.h>

/* Implementation parameters (not user settings). */
#define TORUS_UPDATE_INTERVAL_MS   750   /* idle redraw period */
#define TORUS_ROTATION_SPEED       546   /* idle spin per update, angle units (~3 deg) */
#define TORUS_TRANSITION_INTERVAL_MS 60  /* redraw period while turning toward selection */

#define TORUS_SEGS_PER_CELL 6
#define TORUS_U_SEGS (3 * TORUS_SEGS_PER_CELL)
#define TORUS_V_SEGS (3 * TORUS_SEGS_PER_CELL)
#define TORUS_QUADS (TORUS_U_SEGS * TORUS_V_SEGS)

#define TORUS_MAJOR_R 4096   /* 1.0  */
#define TORUS_MINOR_R 2048   /* 0.5  */
#define TORUS_EXTENT  (TORUS_MAJOR_R + TORUS_MINOR_R)   /* max |x|,|y| of the model */

typedef struct { int16_t x, y, z; } TorusVec;
typedef struct { int16_t m[3][3]; } TorusMat;
typedef struct { uint16_t spin; int32_t pitch; } TorusCamera;   /* pitch in angle units, 0 = axis toward viewer */
typedef struct { int16_t cx, cy, half; } TorusProjection;       /* centre and half-size in pixels */

typedef struct { int16_t x, y; } TorusPt;
typedef struct { uint8_t i, j, cell; uint8_t light; } TorusQuad;  /* light 0..255 */
typedef struct {
    TorusPt  vert[TORUS_U_SEGS][TORUS_V_SEGS];     /* projected grid vertices */
    TorusQuad quads[TORUS_QUADS];                  /* visible quads, back-to-front */
    int      num_quads;
    TorusPt  cell_center[9];
    int16_t  cell_facing[9];                       /* rotated normal z, Q14: >0 faces viewer */
} TorusFrame;

int16_t torus_sin(uint16_t a);   /* Q14 */
int16_t torus_cos(uint16_t a);

void torus_projection_fit(TorusProjection *p, int x, int y, int w, int h);  /* scale to region */
TorusMat torus_camera_matrix(const TorusCamera *c);
void torus_camera_target_for_cell(int cell, TorusCamera *target);
bool torus_camera_step(TorusCamera *cur, const TorusCamera *target);   /* true while still moving */
void torus_camera_idle_step(TorusCamera *cur);
void torus_build_frame(const TorusCamera *cam, const TorusProjection *proj, TorusFrame *out);

#endif
