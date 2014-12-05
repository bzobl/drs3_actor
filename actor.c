#include "actor.h"
#include "msp430.h"

#define PWM_STEP_SIZE          ((pwm_t)400)
#define PWM_DEFAULT_DUTY_CYCLE (PWM_STEP_SIZE/3)  //TODO default?

#define MISSING  0
#define RECEIVED 1

#define NO_ERROR 0

typedef struct {
  uint8_t received;
  uint8_t error;
  pwm_t value;
} VoterValue;

static VoterValue voter_inputs[NUM_OF_SENSORS];

static void pwm_init(void)
{
  /* init Timer/PWM */
  P2SEL |= BIT5; // link pwm to the dout pin
  TA1CCR0 = PWM_STEP_SIZE;
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

void actor_add_voter_value(int sensor_id, uint8_t error, pwm_t value)
{
  voter_inputs[sensor_id_to_idx(sensor_id)].value = value;
  voter_inputs[sensor_id_to_idx(sensor_id)].received = RECEIVED;
  voter_inputs[sensor_id_to_idx(sensor_id)].received = RECEIVED;
}

static void pwm_set_duty_cycle(pwm_t const duty_cycle)
{
  if (duty_cycle >= 0) {
    TA1CCR2 = duty_cycle;
  }
}

bool actor_vote(void)
{
  static pwm_t last_vote = 0;
  static int n_unchanged_votes = 0;

  int n_valid = 0;
  pwm_t duty_cycle = 0;

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
