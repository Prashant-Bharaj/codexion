/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_dongle.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/08 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2026/03/28 00:00:00 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static long	compute_priority(t_sim *sim, int cid)
{
	long	deadline;

	pthread_mutex_lock(&sim->coder_data[cid - 1].mutex);
	deadline = sim->coder_data[cid - 1].last_compile_start
		+ sim->params.time_to_burnout;
	pthread_mutex_unlock(&sim->coder_data[cid - 1].mutex);
	if (sim->params.scheduler == CODEX_FIFO)
		return (get_time_ms());
	return (deadline * (sim->params.num_coders + 1) - cid);
}

static int	phase1_wait(t_sim *sim, int cid, int f)
{
	long			now;
	long			wait_ms;
	struct timespec	ts;

	while (!dongle_request_queue_peek_can_serve(
			sim->dongles[f].request_queue, cid))
	{
		if (is_stopped(sim))
		{
			dongle_request_queue_remove_coder(
				sim->dongles[f].request_queue, cid);
			pthread_mutex_unlock(&sim->dongles[f].mutex);
			return (-1);
		}
		now = get_time_ms();
		wait_ms = pair_wait_ms(sim, f, f, now);
		abs_time_in_ms(wait_ms, &ts);
		pthread_cond_timedwait(&sim->dongles[f].cond,
			&sim->dongles[f].mutex, &ts);
	}
	dongle_request_queue_remove_front(sim->dongles[f].request_queue);
	return (0);
}

static int	p2_stop_cleanup(t_sim *sim, int cid, int f, int s)
{
	pthread_mutex_lock(&sim->dongles[s].mutex);
	dongle_request_queue_remove_coder(
		sim->dongles[s].request_queue_s, cid);
	pthread_mutex_unlock(&sim->dongles[s].mutex);
	pthread_mutex_unlock(&sim->dongles[f].mutex);
	return (-1);
}

static int	phase2_take(t_sim *sim, int cid, int f, int s)
{
	long			now;
	long			wait_ms;
	struct timespec	ts;

	pthread_mutex_lock(&sim->dongles[s].mutex);
	dongle_request_queue_add(sim->dongles[s].request_queue_s,
		cid, compute_priority(sim, cid));
	while (!try_take_and_log(sim, cid, f, s))
	{
		now = get_time_ms();
		wait_ms = pair_wait_ms(sim, f, s, now);
		pthread_mutex_unlock(&sim->dongles[s].mutex);
		abs_time_in_ms(wait_ms, &ts);
		pthread_cond_timedwait(&sim->dongles[f].cond,
			&sim->dongles[f].mutex, &ts);
		if (is_stopped(sim))
			return (p2_stop_cleanup(sim, cid, f, s));
		pthread_mutex_lock(&sim->dongles[s].mutex);
	}
	return (0);
}

int	acquire_two_dongles(t_sim *sim, int cid, int left, int right)
{
	int	f;
	int	s;

	order_indices(left, right, &f, &s);
	pthread_mutex_lock(&sim->dongles[f].mutex);
	dongle_request_queue_add(sim->dongles[f].request_queue,
		cid, compute_priority(sim, cid));
	if (phase1_wait(sim, cid, f) != 0)
		return (-1);
	if (f == s)
		return (take_single(sim, cid, f));
	return (phase2_take(sim, cid, f, s));
}
