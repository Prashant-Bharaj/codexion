/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/05 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2025/02/05 00:00:00 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdlib.h>
#include <sys/time.h>

int	dongle_init(t_dongle *d, void *queue)
{
	if (!d || !queue)
		return (-1);
	if (pthread_mutex_init(&d->mutex, NULL) != 0)
		return (-1);
	if (pthread_cond_init(&d->cond, NULL) != 0)
	{
		pthread_mutex_destroy(&d->mutex);
		return (-1);
	}
	d->cooldown_until = 0;
	d->holder = -1;
	d->request_queue = queue;
	return (0);
}

void	dongle_destroy(t_dongle *d)
{
	if (!d)
		return;
	pthread_cond_destroy(&d->cond);
	pthread_mutex_destroy(&d->mutex);
}

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

static void	abs_time_in_ms(long ms_from_now, struct timespec *ts)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	ts->tv_sec = tv.tv_sec + ms_from_now / 1000;
	ts->tv_nsec = tv.tv_usec * 1000 + (ms_from_now % 1000) * 1000000;
	if (ts->tv_nsec >= 1000000000)
	{
		ts->tv_sec++;
		ts->tv_nsec -= 1000000000;
	}
}

int	dongle_acquire(t_dongle *d, t_sim *sim, int coder_id, long deadline)
{
	long			now;
	long			priority;
	struct timespec	abstime;
	long			wait_ms;

	if (!d || !sim)
		return (-1);
	priority = (sim->params.scheduler == CODEX_FIFO) ? get_time_ms() : deadline;
	pthread_mutex_lock(&d->mutex);
	dongle_request_queue_add(d->request_queue, coder_id, priority);
	while (!should_stop(sim))
	{
		now = get_time_ms();
		if (now >= d->cooldown_until && d->holder < 0
			&& dongle_request_queue_peek_can_serve(d->request_queue, coder_id))
		{
			dongle_request_queue_remove_front(d->request_queue);
			d->holder = coder_id;
			d->cooldown_until = 0;
			pthread_mutex_unlock(&d->mutex);
			return (0);
		}
		if (should_stop(sim))
			break;
		wait_ms = 5;
		if (d->cooldown_until > now)
		{
			wait_ms = d->cooldown_until - now + 1;
			if (wait_ms > 100)
				wait_ms = 100;
		}
		abs_time_in_ms(wait_ms, &abstime);
		pthread_cond_timedwait(&d->cond, &d->mutex, &abstime);
	}
	pthread_mutex_unlock(&d->mutex);
	return (-1);
}

void	dongle_release(t_dongle *d, t_sim *sim)
{
	long	now;

	if (!d || !sim)
		return;
	pthread_mutex_lock(&d->mutex);
	d->holder = -1;
	now = get_time_ms();
	d->cooldown_until = now + sim->params.dongle_cooldown;
	pthread_cond_broadcast(&d->cond);
	pthread_mutex_unlock(&d->mutex);
}

void	signal_stop(t_sim *sim)
{
	int	i;

	if (!sim)
		return;
	pthread_mutex_lock(&sim->stop_mutex);
	sim->stop = 1;
	pthread_mutex_unlock(&sim->stop_mutex);
	i = 0;
	while (i < sim->params.num_coders)
	{
		pthread_mutex_lock(&sim->dongles[i].mutex);
		pthread_cond_broadcast(&sim->dongles[i].cond);
		pthread_mutex_unlock(&sim->dongles[i].mutex);
		i++;
	}
}
