#ifndef ACTOR_H_INCLUDED
#define ACTOR_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#define NUM_OF_SENSORS 3
#define MIN_NODES      2

#define UNCHANGED_VOTE_ERROR_LIMIT 5

typedef uint8_t pwm_t;

/**
 * initializes actor system
 */
void actor_init(void);

/**
 * adds a new value to the next vote
 * @param sensor_id ID of the sensor this value is from
 * @param value value to be voted for
 */
void actor_add_voter_value(int sensor_id, uint8_t error, pwm_t value);

/**
 * initiates voting
 * @return boolean whether or not the vote was successful
 */
bool actor_vote(void);

#endif
