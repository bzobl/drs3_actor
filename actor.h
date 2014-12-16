#ifndef ACTOR_H_INCLUDED
#define ACTOR_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#define NUM_OF_SENSORS 3
#define MIN_NODES      2
#define THRESHOLD	   10

#define UNCHANGED_VOTE_ERROR_LIMIT 5

typedef uint8_t duty_cycle_t;

/**
 * initializes actor system
 */
void actor_init(void);

/**
 * sets the pwm duty cycle manually
 * <p>The PWM has a period of 0.1ms</p>
 * @param duty_cycle the new duty_cycle to be set
 */
void pwm_set_duty_cycle(duty_cycle_t const duty_cycle);

/**
 * sets the pwm compare value directly
 * <p>The PWM has a period of 0.1ms</p>
 * @param compare_value the timer value to change DOUT level at (reset at 0, set at compare_value, high until 400)
 */
void pwm_set_compare_value(uint16_t const compare_value);

/**
 * adds a new value to the next vote
 * @param sensor_id ID of the sensor this value is from
 * @param value value to be voted for
 */
void actor_add_voter_value(int sensor_id, uint8_t error, duty_cycle_t value);

/**
 * initiates voting
 * @return boolean whether or not the vote was successful
 */
bool actor_vote(void);

#endif
