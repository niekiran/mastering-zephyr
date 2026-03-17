#ifndef INCLUDE_RENDER_H_
#define INCLUDE_RENDER_H_

#include "game.h"

/*
 * render_frame() — Process game events and update the display.
 *
 * Call once per tick after game_update(). All event decoding and
 * differential rendering is handled internally.
 *
 * Returns milliseconds the caller should pause before the next frame
 * (0 for normal frames, >0 after a life-lost event).
 */
int render_frame(const struct game_ctx *ctx, game_events_t events,
		 const struct game_event_data *evt);

#endif /* INCLUDE_RENDER_H_ */
