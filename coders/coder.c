/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/05 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2025/02/05 00:00:00 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <unistd.h>
#include <stdlib.h>

static void	update_last_compile(t_coder_data *data, long now)
{
	pthread_mutex_lock(&data->mutex);
	data->last_compile_start = now;
	data->compile_count++;
	pthread_mutex_unlock(&data->mutex);
}

static long	get_deadline(t_coder_data *data)
{
	long	deadline;

	pthread_mutex_lock(&data->mutex);
	deadline = data->last_compile_start;
	pthread_mutex_unlock(&data->mutex);
	return (deadline);
}

static int	acquire_two_dongles(t_sim *sim, int coder_id, int left_idx,
	int right_idx)
{
	int		first;
	int		second;
	long	deadline;

	deadline = get_deadline(&sim->coder_data[coder_id - 1])
		+ sim->params.time_to_burnout;
	if (left_idx <= right_idx)
	{
		first = left_idx;
		second = right_idx;
	}
	else
	{
		first = right_idx;
		second = left_idx;
	}
	if (dongle_acquire(&sim->dongles[first], sim, coder_id, deadline) != 0)
		return (-1);
	safe_log(sim, coder_id, "has taken a dongle");
	if (first != second)
	{
		if (dongle_acquire(&sim->dongles[second], sim, coder_id, deadline) != 0)
		{
			dongle_release(&sim->dongles[first], sim);
			return (-1);
		}
		safe_log(sim, coder_id, "has taken a dongle");
	}
	else
		safe_log(sim, coder_id, "has taken a dongle");
	return (0);
}

static void	release_two_dongles(t_sim *sim, int left_idx, int right_idx)
{
	int	first;
	int	second;

	if (left_idx <= right_idx)
	{
		first = left_idx;
		second = right_idx;
	}
	else
	{
		first = right_idx;
		second = left_idx;
	}
	dongle_release(&sim->dongles[first], sim);
	if (first != second)
		dongle_release(&sim->dongles[second], sim);
}

void	*coder_routine(void *arg)
{
	t_coder_arg		*carg;
	t_sim			*sim;
	int				coder_id;
	int				left_idx;
	int				right_idx;
	long			now;

	carg = (t_coder_arg *)arg;
	sim = carg->sim;
	coder_id = carg->coder_id;
	left_idx = get_left_dongle(coder_id, sim->params.num_coders);
	right_idx = get_right_dongle(coder_id, sim->params.num_coders);
	while (1)
	{
		if (acquire_two_dongles(sim, coder_id, left_idx, right_idx) != 0)
			return (NULL);
		now = get_time_ms();
		update_last_compile(&sim->coder_data[coder_id - 1], now);
		safe_log(sim, coder_id, "is compiling");
		usleep((useconds_t)sim->params.time_to_compile * 1000);
		release_two_dongles(sim, left_idx, right_idx);
		safe_log(sim, coder_id, "is debugging");
		usleep((useconds_t)sim->params.time_to_debug * 1000);
		safe_log(sim, coder_id, "is refactoring");
		usleep((useconds_t)sim->params.time_to_refactor * 1000);
		pthread_mutex_lock(&sim->stop_mutex);
		if (sim->coder_data[coder_id - 1].compile_count
			>= sim->params.num_compiles_required)
		{
			sim->num_coders_finished++;
			if (sim->num_coders_finished >= sim->params.num_coders)
			{
				pthread_mutex_unlock(&sim->stop_mutex);
				signal_stop(sim);
				return (NULL);
			}
			pthread_mutex_unlock(&sim->stop_mutex);
			return (NULL);
		}
		pthread_mutex_unlock(&sim->stop_mutex);
	}
	return (NULL);
}
