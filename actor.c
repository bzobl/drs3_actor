#include "actor.h"
#include <msp430.h>

#define PWM_PERIOD_STEPS       ((uint16_t)400)
#define PWM_DEFAULT_COMPARE_VALUE ((uint16_t)0)

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
  TA1CCR2 = PWM_DEFAULT_COMPARE_VALUE;
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
  return (id & 0xF);
}

void actor_add_voter_value(int sensor_id, uint8_t error, duty_cycle_t value)
{
  voter_inputs[sensor_id_to_idx(sensor_id)].value = value;
  voter_inputs[sensor_id_to_idx(sensor_id)].error = error;
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
      pwm_set_compare_value((int) compare_value);
      }
  }
}

bool compare_values(bool *valid, duty_cycle_t *voted_cycle)
{
	uint16_t voted_value = 0;
	bool take_value[NUM_OF_SENSORS][NUM_OF_SENSORS];
	uint8_t max_taken = 0;
	uint8_t max_taken_idx = 0;

	for (int i = 0; i < NUM_OF_SENSORS; i++) {
		uint8_t taken = 0;
		uint8_t lower_range = (voter_inputs[i].value >= THRESHOLD) ? (voter_inputs[i].value - THRESHOLD) : 0;
		uint8_t upper_range = (voter_inputs[i].value <= (255 - THRESHOLD)) ? (voter_inputs[i].value + THRESHOLD) : 255;

		if (valid[i]) {
			for (int j = 0; j < NUM_OF_SENSORS; j++) {
				if (valid[j]) {
					if (i == j) {
						take_value[i][j] = true;
						taken++;
					} else {
						uint8_t their_value = voter_inputs[j].value;

						if ((their_value >= lower_range) && (their_value <= upper_range)) {
							take_value[i][j] = true;
							taken++;
						} else {
							take_value[i][j] = false;
						}
					}
				}
			}

			if (taken > max_taken) {
				max_taken = taken;
				max_taken_idx = i;
			}
		}
	}

	if (max_taken < MIN_NODES) {
		// Too few nodes to compare
		return false;
	}

	// average of all values in THRESHOLD
	for (int i = 0; i < NUM_OF_SENSORS; i++) {
		if (take_value[max_taken_idx][i]) {
			voted_value += voter_inputs[i].value;
		}
	}

	voted_value /= max_taken;

	*voted_cycle = (duty_cycle_t) voted_value;
	return true;
}

bool actor_vote(void)
{
  static duty_cycle_t last_vote = 0;
  static int n_unchanged_votes = 0;

  int n_valid = 0;
  bool valid[NUM_OF_SENSORS] = { false };
  duty_cycle_t duty_cycle = 0;

  for (int i = 0; i < NUM_OF_SENSORS; i++) {
    if (voter_inputs[i].received == MISSING) {
      continue;
    }

    // reset received flag for next round
    voter_inputs[i].received = MISSING;

    if (voter_inputs[i].error != NO_ERROR) {
    	continue;
    }

    n_valid++;
    valid[i] = true;
  }

  if (n_valid < MIN_NODES) {
	pwm_set_compare_value(PWM_DEFAULT_COMPARE_VALUE);
    return false;
  }

  if (!compare_values(valid, &duty_cycle)) {
	pwm_set_compare_value(PWM_DEFAULT_COMPARE_VALUE);
	return false;
  }

  pwm_set_duty_cycle(duty_cycle);

  // check whether the duty_cycle has changed
  if (duty_cycle == last_vote) {
    n_unchanged_votes++;
  } else {
    last_vote = duty_cycle;
    n_unchanged_votes = 0;
  }

  if (n_unchanged_votes >= UNCHANGED_VOTE_ERROR_LIMIT) {
	pwm_set_compare_value(PWM_DEFAULT_COMPARE_VALUE);
    return false;
  }

  return true;
}
