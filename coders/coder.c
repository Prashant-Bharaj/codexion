/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/08 11:03:13 by prasingh          #+#    #+#             */
/*   Updated: 2026/02/16 15:50:25 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdlib.h>
#include <unistd.h>

static void	update_last_compile(t_coder_data *data, long now)
{
	pthread_mutex_lock(&data->mutex);
	data->last_compile_start = now;
	data->compile_count++;
	pthread_mutex_unlock(&data->mutex);
}

static int	check_done(t_sim *sim, int coder_id)
{
	int	done;
	int	compile_count;

	done = 0;
	pthread_mutex_lock(&sim->coder_data[coder_id - 1].mutex);
	compile_count = sim->coder_data[coder_id - 1].compile_count;
	pthread_mutex_unlock(&sim->coder_data[coder_id - 1].mutex);
	if (compile_count >= sim->params.num_compiles_required)
	{
		pthread_mutex_lock(&sim->stop_mutex);
		sim->num_coders_finished++;
		if (sim->num_coders_finished >= sim->params.num_coders)
			done = 2;
		else
			done = 1;
		pthread_mutex_unlock(&sim->stop_mutex);
	}
	return (done);
}

static int	do_one_compile(t_sim *sim, int coder_id, int left_idx,
		int right_idx)
{
	if (acquire_two_dongles(sim, coder_id, left_idx, right_idx) != 0)
		return (-1);
	update_last_compile(&sim->coder_data[coder_id - 1], get_time_ms());
	safe_log(sim, coder_id, "is compiling");
	usleep((useconds_t)sim->params.time_to_compile * 1000);
	release_two_dongles(sim, left_idx, right_idx);
	safe_log(sim, coder_id, "is debugging");
	usleep((useconds_t)sim->params.time_to_debug * 1000);
	safe_log(sim, coder_id, "is refactoring");
	usleep((useconds_t)sim->params.time_to_refactor * 1000);
	return (0);
}

void	*coder_routine(void *arg)
{
	t_sim	*sim;
	int		coder_id;
	int		left_idx;
	int		right_idx;
	int		done;
	int		should_stop;

	sim = ((t_coder_arg *)arg)->sim;
	coder_id = ((t_coder_arg *)arg)->coder_id;
	left_idx = get_left_dongle(coder_id, sim->params.num_coders);
	right_idx = get_right_dongle(coder_id, sim->params.num_coders);
	if (coder_id % 2 == 0)
		usleep(10000);
	while (1)
	{
		pthread_mutex_lock(&sim->stop_mutex);
		should_stop = sim->stop;
		pthread_mutex_unlock(&sim->stop_mutex);
		if (should_stop)
			return (NULL);
		if (do_one_compile(sim, coder_id, left_idx, right_idx) != 0)
			return (NULL);
		done = check_done(sim, coder_id);
		if (done != 0)
		{
			if (done == 2)
				signal_stop(sim);
			return (NULL);
		}
	}
	return (NULL);
}
