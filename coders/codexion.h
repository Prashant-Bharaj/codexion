/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/05 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2025/02/05 00:00:00 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <stddef.h>

# define CODEX_FIFO 0
# define CODEX_EDF  1

typedef struct s_sim	t_sim;

typedef struct s_coder_arg
{
	t_sim	*sim;
	int		coder_id;
}	t_coder_arg;

typedef struct s_params
{
	int		num_coders;
	long	time_to_burnout;
	long	time_to_compile;
	long	time_to_debug;
	long	time_to_refactor;
	int		num_compiles_required;
	long	dongle_cooldown;
	int		scheduler;
}	t_params;

typedef struct s_coder_data
{
	int				id;
	long			last_compile_start;
	int				compile_count;
	pthread_mutex_t	mutex;
}	t_coder_data;

typedef struct s_dongle
{
	pthread_mutex_t		mutex;
	pthread_cond_t		cond;
	long				cooldown_until;
	int					holder;
	void				*request_queue;
}	t_dongle;

struct s_sim
{
	t_params		params;
	t_dongle		*dongles;
	t_coder_data	*coder_data;
	long			start_time;
	int				stop;
	int				burnout_coder;
	int				num_coders_finished;
	pthread_mutex_t	log_mutex;
	pthread_mutex_t	stop_mutex;
};

long	get_time_ms(void);
long	elapsed_ms(long start);
void	safe_log(t_sim *sim, int coder_id, const char *msg);

int		parse_args(int argc, char **argv, t_params *params);

void	*dongle_request_queue_create(int scheduler);
void	dongle_request_queue_destroy(void *queue);
void	dongle_request_queue_add(void *queue, int coder_id, long priority);
int		dongle_request_queue_remove_front(void *queue);
int		dongle_request_queue_peek_can_serve(void *queue, int coder_id);

int		dongle_init(t_dongle *d, void *queue);
void	dongle_destroy(t_dongle *d);
int		dongle_acquire(t_dongle *d, t_sim *sim, int coder_id, long deadline);
void	dongle_release(t_dongle *d, t_sim *sim);
void	signal_stop(t_sim *sim);

void	*coder_routine(void *arg);
void	*monitor_routine(void *arg);

int		get_left_dongle(int coder_id, int num_coders);
int		get_right_dongle(int coder_id, int num_coders);

#endif
