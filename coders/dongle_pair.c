/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_pair.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/28 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2026/03/28 00:00:00 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	try_take_pair(t_sim *sim, int cid, int f, int s)
{
	long	now;

	now = get_time_ms();
	if (now < sim->dongles[f].cooldown_until
		|| sim->dongles[f].holder >= 0)
		return (0);
	if (now < sim->dongles[s].cooldown_until
		|| sim->dongles[s].holder >= 0
		|| !dongle_request_queue_peek_can_serve(
			sim->dongles[s].request_queue_s, cid))
		return (0);
	dongle_request_queue_remove_front(sim->dongles[s].request_queue_s);
	sim->dongles[f].holder = cid;
	sim->dongles[s].holder = cid;
	return (1);
}

int	try_take_and_log(t_sim *sim, int cid, int f, int s)
{
	if (!try_take_pair(sim, cid, f, s))
		return (0);
	pthread_mutex_unlock(&sim->dongles[s].mutex);
	pthread_mutex_unlock(&sim->dongles[f].mutex);
	safe_log(sim, cid, "has taken a dongle");
	safe_log(sim, cid, "has taken a dongle");
	return (1);
}

int	take_single(t_sim *sim, int cid, int f)
{
	long			wait_ms;
	struct timespec	ts;

	while (get_time_ms() < sim->dongles[f].cooldown_until)
	{
		if (is_stopped(sim))
		{
			pthread_mutex_unlock(&sim->dongles[f].mutex);
			return (-1);
		}
		wait_ms = pair_wait_ms(sim, f, f, get_time_ms());
		abs_time_in_ms(wait_ms, &ts);
		pthread_cond_timedwait(&sim->dongles[f].cond,
			&sim->dongles[f].mutex, &ts);
	}
	sim->dongles[f].holder = cid;
	pthread_mutex_unlock(&sim->dongles[f].mutex);
	safe_log(sim, cid, "has taken a dongle");
	safe_log(sim, cid, "has taken a dongle");
	return (0);
}

void	release_two_dongles(t_sim *sim, int left_idx, int right_idx)
{
	int	first;
	int	second;

	order_indices(left_idx, right_idx, &first, &second);
	dongle_release(&sim->dongles[first], sim);
	if (first != second)
		dongle_release(&sim->dongles[second], sim);
}
