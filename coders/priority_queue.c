/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   priority_queue.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/05 00:00:00 by prasingh          #+#    #+#             */
/*   Updated: 2025/02/05 00:00:00 by prasingh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdlib.h>

typedef struct s_pq_node
{
	int		coder_id;
	long	priority;
}	t_pq_node;

typedef struct s_priority_queue
{
	t_pq_node	*nodes;
	int			capacity;
	int			size;
	int			scheduler;
}	t_priority_queue;

static void	heapify_up(t_priority_queue *pq, int idx)
{
	int			parent;
	t_pq_node	tmp;

	while (idx > 0)
	{
		parent = (idx - 1) / 2;
		if (pq->nodes[idx].priority >= pq->nodes[parent].priority)
			break;
		tmp = pq->nodes[idx];
		pq->nodes[idx] = pq->nodes[parent];
		pq->nodes[parent] = tmp;
		idx = parent;
	}
}

static void	heapify_down(t_priority_queue *pq, int idx)
{
	int			left;
	int			right;
	int			smallest;
	t_pq_node	tmp;

	while (1)
	{
		left = 2 * idx + 1;
		right = 2 * idx + 2;
		smallest = idx;
		if (left < pq->size && pq->nodes[left].priority < pq->nodes[smallest].priority)
			smallest = left;
		if (right < pq->size && pq->nodes[right].priority < pq->nodes[smallest].priority)
			smallest = right;
		if (smallest == idx)
			break;
		tmp = pq->nodes[idx];
		pq->nodes[idx] = pq->nodes[smallest];
		pq->nodes[smallest] = tmp;
		idx = smallest;
	}
}

void	*dongle_request_queue_create(int scheduler)
{
	t_priority_queue	*pq;

	pq = (t_priority_queue *)malloc(sizeof(t_priority_queue));
	if (!pq)
		return (NULL);
	pq->capacity = 64;
	pq->size = 0;
	pq->scheduler = scheduler;
	pq->nodes = (t_pq_node *)malloc(sizeof(t_pq_node) * pq->capacity);
	if (!pq->nodes)
	{
		free(pq);
		return (NULL);
	}
	return (pq);
}

void	dongle_request_queue_destroy(void *queue)
{
	t_priority_queue	*pq;

	if (!queue)
		return;
	pq = (t_priority_queue *)queue;
	free(pq->nodes);
	free(pq);
}

void	dongle_request_queue_add(void *queue, int coder_id, long priority)
{
	t_priority_queue	*pq;
	t_pq_node			*nodes;

	if (!queue)
		return;
	pq = (t_priority_queue *)queue;
	if (pq->size >= pq->capacity)
	{
		nodes = (t_pq_node *)malloc(sizeof(t_pq_node) * pq->capacity * 2);
		if (!nodes)
			return;
		{
			int	i;

			i = 0;
			while (i < pq->size)
			{
				nodes[i] = pq->nodes[i];
				i++;
			}
		}
		free(pq->nodes);
		pq->nodes = nodes;
		pq->capacity *= 2;
	}
	pq->nodes[pq->size].coder_id = coder_id;
	pq->nodes[pq->size].priority = priority;
	heapify_up(pq, pq->size);
	pq->size++;
}

int	dongle_request_queue_remove_front(void *queue)
{
	t_priority_queue	*pq;
	int					coder_id;

	if (!queue)
		return (-1);
	pq = (t_priority_queue *)queue;
	if (pq->size == 0)
		return (-1);
	coder_id = pq->nodes[0].coder_id;
	pq->size--;
	if (pq->size > 0)
	{
		pq->nodes[0] = pq->nodes[pq->size];
		heapify_down(pq, 0);
	}
	return (coder_id);
}

int	dongle_request_queue_peek_can_serve(void *queue, int coder_id)
{
	t_priority_queue	*pq;

	if (!queue)
		return (0);
	pq = (t_priority_queue *)queue;
	if (pq->size == 0)
		return (0);
	return (pq->nodes[0].coder_id == coder_id);
}
