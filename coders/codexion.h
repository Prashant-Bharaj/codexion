/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/05 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2026/02/08 13:47:58 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <stddef.h>
# include <sys/time.h>

# define CODEX_FIFO 0
# define CODEX_EDF 1

typedef struct s_sim	t_sim;

typedef struct s_coder_arg
{
	t_sim				*sim;
	int					coder_id;
}						t_coder_arg;

typedef struct s_params
{
	int					num_coders;
	long				time_to_burnout;
	long				time_to_compile;
	long				time_to_debug;
	long				time_to_refactor;
	int					num_compiles_required;
	long				dongle_cooldown;
	int					scheduler;
}						t_params;

typedef struct s_coder_data
{
	int					id;
	long				last_compile_start;
	int					compile_count;
	pthread_mutex_t		mutex;
}						t_coder_data;

typedef struct s_dongle
{
	pthread_mutex_t		mutex;
	pthread_cond_t		cond;
	long				cooldown_until;
	int					holder;
	void				*request_queue;
}						t_dongle;

struct					s_sim
{
	t_params			params;
	t_dongle			*dongles;
	t_coder_data		*coder_data;
	long				start_time;
	int					stop;
	int					burnout_coder;
	int					num_coders_finished;
	pthread_mutex_t		log_mutex;
	pthread_mutex_t		stop_mutex;
};

long					get_time_ms(void);
int						is_stopped(t_sim *sim);
void					wake_all_dongles(t_sim *sim);
void					safe_log(t_sim *sim, int coder_id, const char *msg);

int						parse_args(int argc, char **argv, t_params *params);
int						dongle_init_sim(t_dongle *d, void *queue);
void					dongle_destroy_sim(t_dongle *d);
int						init_mutexes(t_sim *sim);
int						alloc_dongles(t_sim *sim);
int						alloc_coder_data(t_sim *sim);
int						init_simulation(t_sim *sim);
void					cleanup_simulation(t_sim *sim);
int						parse_and_init(t_sim *sim, int argc, char **argv);
int						run_simulation(t_sim *sim);

void					*dongle_request_queue_create(int scheduler);
void					dongle_request_queue_destroy(void *queue);
void					dongle_request_queue_add(void *queue, int coder_id,
							long priority);
int						dongle_request_queue_remove_front(void *queue);
int						dongle_request_queue_peek_can_serve(void *queue,
							int coder_id);

void					abs_time_in_ms(long ms_from_now, struct timespec *ts);
void					dongle_release(t_dongle *d, t_sim *sim);
void					signal_stop(t_sim *sim);

int						acquire_two_dongles(t_sim *sim, int coder_id,
							int left_idx, int right_idx);
void					release_two_dongles(t_sim *sim, int left_idx,
							int right_idx);
void					*coder_routine(void *arg);
void					*monitor_routine(void *arg);

int						get_left_dongle(int coder_id, int num_coders);
int						get_right_dongle(int coder_id, int num_coders);
void					order_indices(int left, int right, int *first,
							int *second);
void					get_coder_dongles(int coder_id, int num_coders,
							int *left_idx, int *right_idx);

typedef struct s_pq_node
{
	int					coder_id;
	long				priority;
}						t_pq_node;

typedef struct s_priority_queue
{
	t_pq_node			*nodes;
	int					capacity;
	int					size;
	int					scheduler;
}						t_priority_queue;

void					heapify_up(t_priority_queue *pq, int idx);
void					heapify_down(t_priority_queue *pq, int idx);
int						grow_queue(t_priority_queue *pq);

#endif
