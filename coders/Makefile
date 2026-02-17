# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: prasingh <prasingh@student.42berlin.de>    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/02/05 00:00:00 by prasingh          #+#    #+#              #
#    Updated: 2025/02/05 00:00:00 by prasingh         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME		= codexion

SRCS		= main.c main_parse.c main_run.c utils.c parse_args.c \
			  dongle_time.c coder.c coder_dongle.c sim_init.c sim_alloc.c \
			  sim_cleanup.c sim_dongle.c monitor.c priority_queue.c pq_heap.c \
			  geometry.c

OBJS		= $(SRCS:.c=.o)

CC			= cc
CFLAGS		= -Wall -Wextra -Werror -pthread
INCLUDE		= -I.

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^

%.o: %.c codexion.h
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
