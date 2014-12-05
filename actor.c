#include "actor.h"
#include <msp430.h>

#define PWM_PERIOD_STEPS       ((uint16_t)400)
#define PWM_DEFAULT_DUTY_CYCLE ((uint16_t)0)

#define MISSING  0
#define RECEIVED 1

#define NO_ERROR 0

typedef struct {
  uint8_t received;
  uint8_t error;
  duty_cycle_t value;
} VoterValue;

static VoterValue voter_inputs[NUM_OF_SENSORS];

static void pwm_init(void)
{
  /* init Timer/PWM */
  P2SEL |= BIT5; // link pwm to the dout pin
  TA1CCR0 = 400;
  TA1CCR2 = PWM_DEFAULT_DUTY_CYCLE;
  TA1CTL = TASSEL_2 + MC_1;
  TA1CCTL2 = OUTMOD_7;
}

void actor_init(void)
{
  for (int i = 0; i < NUM_OF_SENSORS; i++) {
    voter_inputs[i].received = MISSING;
  }

  pwm_init();
}

/*
 * in case the sensor ids are not 0 to NUM_OF_SENSORS - 1
 * the id has to be transformed to an idx
 * e.g. by calculating or looking up
 */
static inline int sensor_id_to_idx(int id)
{
  return id;
}

void actor_add_voter_value(int sensor_id, uint8_t error, duty_cycle_t value)
{
  voter_inputs[sensor_id_to_idx(sensor_id)].value = value;
  voter_inputs[sensor_id_to_idx(sensor_id)].received = RECEIVED;
  voter_inputs[sensor_id_to_idx(sensor_id)].received = RECEIVED;
}

void pwm_set_compare_value(uint16_t const compare_value)
{
  TA1CCR2 = compare_value;
}

void pwm_set_duty_cycle(duty_cycle_t const duty_cycle)
{
  switch (duty_cycle) {
  case 0x00:
	  pwm_set_compare_value(0);
	  break;

  case 0xFF:
	  pwm_set_compare_value(PWM_PERIOD_STEPS);
	  break;

  default:
    {
    //FIXME: floating point calculation does not work on MSP430
    // find a way to efficiently map the duty_cycle to the compare value
    float compare_value = (float)duty_cycle * 1.5625f;
    pwm_set_compare_value(compare_value);
    }
  }
}

bool actor_vote(void)
{
  static duty_cycle_t last_vote = 0;
  static int n_unchanged_votes = 0;

  int n_valid = 0;
  duty_cycle_t duty_cycle = 0;

  for (int i = 0; i < NUM_OF_SENSORS; i++) {
    if (voter_inputs[i].received == MISSING) {
      continue;
    }

    if (voter_inputs[i].error != NO_ERROR) {

    }

    // reset received flag for next round
    voter_inputs[i].received = MISSING;
    n_valid++;
  }

  if (n_valid < MIN_NODES) {
    return false;
  }

  // TODO vote
  // TODO set new pwm duty cycle

  // check whether the duty_cycle has changed
  if (duty_cycle == last_vote) {
    n_unchanged_votes++;
  } else {
    last_vote = duty_cycle;
  }

  if (n_unchanged_votes >= UNCHANGED_VOTE_ERROR_LIMIT) {
    // TODO error handling
  }

  return true;
}
