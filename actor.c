#include "actor.h"
#include "msp430.h"

#define PWM_STEP_SIZE          ((pwm_t)200)
#define PWM_DEFAULT_DUTY_CYCLE (PWM_STEP_SIZE/3)  //TODO default?

#define INVALID 0;
#define VALID 1;

typedef struct {
  uint8_t valid;
  pwm_t value;
} VoterValue;

static VoterValue voter_inputs[NUM_OF_SENSORS];

static void pwm_init(void)
{
  /* init Timer/PWM */
  P2SEL |= BIT5; // link pwm to the dout pin
  TA1CCR0 = PWM_STEP_SIZE;
  TA1CCR2 = 0;
  TA1CTL = TASSEL_2 + MC_1;
  TA1CCTL2 = OUTMOD_7;
}

void actor_init(void)
{
  for (int i = 0; i < NUM_OF_SENSORS; i++) {
    voter_inputs[i].valid = INVALID;
  }
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

void actor_add_voter_value(int sensor_id, pwm_t value)
{
  voter_inputs[sensor_id_to_idx(sensor_id)].value = value;
  voter_inputs[sensor_id_to_idx(sensor_id)].valid = VALID;
}

static void pwm_set_duty_cycle(pwm_t const duty_cycle)
{
  if (dutyCycle >= 0) {
    TA1CCR2 = dutyCycle;
  }
}

bool actor_vote(void)
{
  int n_valid = 0;

  for (int i = 0; i < NUM_OF_SENSORS; i++) {
    if (voter_inputs[i].valid == INVALID) {
      continue;
    }

    // reset valid flag for next round
    voter_inputs[i].valid = INVALID;
    n_valid++;
  }

  if (n_valid < MIN_NODES) {
    return false;
  }

  return true;
}
