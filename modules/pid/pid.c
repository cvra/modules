#include <string.h>
#include <pid.h>

/**
 *  signed maxmimum : both signs are tested
 */
#define S_MAX(to_saturate, value_max)    \
do {                                     \
   if (to_saturate > value_max)          \
     to_saturate = value_max;            \
   else if (to_saturate < -value_max)    \
     to_saturate = -value_max;           \
 } while(0)

/** this function will initialize all fieds of pid structure to 0 */
void pid_init(struct pid_filter *p)
{
    memset(p, 0, sizeof(*p));
    p->gain_P = 1;
    p->derivate_nb_samples = 1;
}

/** this function will initialize all fieds of pid structure to 0,
 *  except configuration */
void pid_reset(struct pid_filter *p)
{
    memset(p->prev_samples, 0, sizeof(p->prev_samples));
    p->integral = 0;
    p->prev_D = 0;
    p->prev_out = 0;
}

void pid_set_gains(struct pid_filter *p, int16_t gp, int16_t gi, int16_t gd)
{
    p->gain_P  = gp;
    p->gain_I  = gi;
    p->gain_D  = gd;
}

void pid_set_maximums(struct pid_filter *p, int32_t max_in, int32_t max_I, int32_t max_out)
{
    p->max_in  = max_in;
    p->max_I   = max_I;
    p->max_out = max_out;
}

void pid_set_out_shift(struct pid_filter *p, uint8_t out_shift)
{
    p->out_shift=out_shift;
}

int8_t pid_set_derivate_filter(struct pid_filter *p, uint8_t nb_samples)
{
    int8_t ret;
    if (nb_samples > PID_DERIVATE_FILTER_MAX_SIZE) {
        ret = -1;
    } else {
        p->derivate_nb_samples = nb_samples;
        ret = 0;
    }
    return ret;
}

int16_t pid_get_gain_P(struct pid_filter *p)
{
    return (p->gain_P);
}

int16_t pid_get_gain_I(struct pid_filter *p)
{
    return (p->gain_I);
}

int16_t pid_get_gain_D(struct pid_filter *p)
{
    return (p->gain_D);
}


int32_t pid_get_max_in(struct pid_filter *p)
{
    return (p->max_in);
}

int32_t pid_get_max_I(struct pid_filter *p)
{
    return (p->max_I);
}

int32_t pid_get_max_out(struct pid_filter *p)
{
    return (p->max_out);
}


uint8_t pid_get_out_shift(struct pid_filter *p)
{
    return (p->out_shift);
}

uint8_t pid_get_derivate_filter(struct pid_filter *p)
{
    return (p->derivate_nb_samples);
}

int32_t pid_get_value_I(struct pid_filter *p)
{
    return (p->integral);
}

int32_t pid_get_value_in(struct pid_filter *p)
{
    return p->prev_samples[p->index];
}

int32_t pid_get_value_D(struct pid_filter *p)
{
    return p->prev_D;
}

int32_t pid_get_value_out(struct pid_filter *p)
{
    return (p->prev_out);
}

/* first parameter should be a (struct pid_filter *) */
int32_t pid_do_filter(void * data, int32_t in)
{
    int32_t derivate;
    int32_t command;
    struct pid_filter * p = data;
    uint8_t prev_index;

    /*
     * Integral value : the integral become bigger with time .. (think
     * to area of graph, we add one area to the previous) so,
     * integral = previous integral + current value
     */

    /* derivate value
    *             f(t+h) - f(t)        with f(t+h) = current value
    *  derivate = -------------             f(t)   = previous value
    *                    h
    * so derivate = current error - previous error
    *
    * We can apply a filter to reduce noise on the derivate term,
    * by using a bigger period.
    */

    prev_index = p->index + 1;
    if (prev_index >= p->derivate_nb_samples)
        prev_index = 0;

    /* saturate input... it influences integral an derivate */
    if (p->max_in)
        S_MAX(in, p->max_in);

    derivate = in - p->prev_samples[prev_index];
    p->integral += in;

    if (p->max_I)
        S_MAX(p->integral, p->max_I);

    /* so, command = P.coef_P + I.coef_I + D.coef_D */
    command = in * p->gain_P +
        p->integral * p->gain_I +
        (derivate * p->gain_D) / p->derivate_nb_samples;

    if ( command < 0 )
        command = -( -command >> p->out_shift );
    else
        command = command >> p->out_shift;

    if (p->max_out)
        S_MAX (command, p->max_out);


    /* backup of current error value (for the next calcul of derivate value) */
    p->prev_samples[p->index] = in;
    p->index = prev_index; /* next index is prev_index */
    p->prev_D = derivate;
    p->prev_out = command;

    return command;
}
