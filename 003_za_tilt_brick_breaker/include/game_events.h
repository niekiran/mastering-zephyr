#ifndef INCLUDE_GAME_EVENTS_H_
#define INCLUDE_GAME_EVENTS_H_

#include <stdint.h>

/* Event flag bits — multiple events fire per tick, OR them together */
#define EVT_BALL_MOVED    0x01
#define EVT_PADDLE_MOVED  0x02
#define EVT_BRICK_HIT     0x04
#define EVT_SCORE_CHANGED 0x08
#define EVT_LIFE_LOST     0x10
#define EVT_GAME_WON      0x20
#define EVT_GAME_LOST     0x40

typedef uint8_t game_events_t;

/* Per-tick event data — filled by game_update(), consumed by render */
struct game_event_data {
	int old_ball_x, old_ball_y;
	int old_paddle_x;
	int hit_row, hit_col;
	int score;
	int lives;
};

#endif /* INCLUDE_GAME_EVENTS_H_ */
