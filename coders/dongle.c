/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/08 11:03:31 by prasingh          #+#    #+#             */
/*   Updated: 2026/02/08 11:07:49 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdlib.h>
#include <sys/time.h>

static int	should_stop(t_sim *sim)
{
	int	stop;

	if (!sim)
		return (1);
	pthread_mutex_lock(&sim->stop_mutex);
	stop = sim->stop;
	pthread_mutex_unlock(&sim->stop_mutex);
	return (stop);
}

static int	try_acquire(t_dongle *d, int coder_id, long now)
{
	if (now >= d->cooldown_until && d->holder < 0
		&& dongle_request_queue_peek_can_serve(d->request_queue, coder_id))
	{
		dongle_request_queue_remove_front(d->request_queue);
		d->holder = coder_id;
		d->cooldown_until = 0;
		return (0);
	}
	return (-1);
}

static int	acquire_wait_loop(t_dongle *d, t_sim *sim, int coder_id)
{
	long			now;
	struct timespec	abstime;

	while (!should_stop(sim))
	{
		now = get_time_ms();
		if (try_acquire(d, coder_id, now) == 0)
			return (0);
		abs_time_in_ms(calc_wait_ms(d->cooldown_until, now), &abstime);
		pthread_cond_timedwait(&d->cond, &d->mutex, &abstime);
	}
	return (-1);
}

int	dongle_acquire(t_dongle *d, t_sim *sim, int coder_id, long deadline)
{
	long	priority;
	int		n;
	int		acquired;

	if (!d || !sim)
		return (-1);
	if (sim->params.scheduler == CODEX_FIFO)
		priority = get_time_ms();
	else
	{
		n = sim->params.num_coders + 1;
		/* EDF: lower deadline first; tie-break: higher coder_id first */
		priority = deadline * (long)n + (long)(n - coder_id);
	}
	pthread_mutex_lock(&d->mutex);
	dongle_request_queue_add(d->request_queue, coder_id, priority);
	acquired = acquire_wait_loop(d, sim, coder_id);
	pthread_mutex_unlock(&d->mutex);
	if (acquired == 0)
		return (0);
	return (-1);
}
