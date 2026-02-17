/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sim_cleanup.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/08 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2026/02/08 00:00:00 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdlib.h>

static void	cleanup_coder_data(t_sim *sim)
{
	int	i;

	if (!sim->coder_data)
		return ;
	i = 0;
	while (i < sim->params.num_coders)
	{
		pthread_mutex_destroy(&sim->coder_data[i].mutex);
		i++;
	}
	free(sim->coder_data);
}

static void	cleanup_dongles(t_sim *sim)
{
	int	i;

	if (!sim->dongles)
		return ;
	i = 0;
	while (i < sim->params.num_coders)
	{
		dongle_destroy_sim(&sim->dongles[i]);
		dongle_request_queue_destroy(sim->dongles[i].request_queue);
		i++;
	}
	free(sim->dongles);
}

void	cleanup_simulation(t_sim *sim)
{
	if (!sim)
		return ;
	cleanup_coder_data(sim);
	cleanup_dongles(sim);
	pthread_mutex_destroy(&sim->stop_mutex);
	pthread_mutex_destroy(&sim->log_mutex);
}
