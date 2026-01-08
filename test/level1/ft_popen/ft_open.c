/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_open.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jjorda <jjorda@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/08 11:29:34 by jjorda            #+#    #+#             */
/*   Updated: 2026/01/08 13:22:46 by jjorda           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

static int	close_fd(int fd[], int fd_0, int fd_1)
{
	close(fd[fd_0]);
	close(fd[fd_1]);
	return (-1);
}

static int	return_type(char type, int fd[])
{
	if (type == 'r')
	{
		close(fd[1]);
		return (fd[0]);
	}
	else
	{
		close(fd[0]);
		return (fd[1]);
	}
}

int	ft_popen(const char *file, char *const argv[], char type)
{
	int		fd[2];
	pid_t	pid;

	if (!file || !argv || (type != 'r' && type != 'w') || pipe(fd) == -1)
		return (-1);
	if ((pid = fork()) == -1)
		close_fd(fd, 0, 1);
	if (pid == 0)
	{
		if (type == 'r')
			if (dup2(fd[1], STDOUT_FILENO) == -1)
				exit(1);
		else
			if (dup2(fd[0], STDIN_FILENO) == -1)
				exit(1);
		close_fd(fd, 0, 1);
		execvp(file, argv);
		exit(1);
	}
	return (return_type(type, fd));
}
