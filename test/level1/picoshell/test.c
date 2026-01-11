/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jjorda <jjorda@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/11 12:39:09 by jjorda            #+#    #+#             */
/*   Updated: 2026/01/11 13:03:42 by jjorda           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

int	ft_ret(int fd[], int prev, int next)
{
	if (next)
	{
		close(fd[0]);
		close(fd[1]);
	}
	if (prev != -1)
		close(prev);
	return (1);
}

void	ft_child(char **cmd, int fd[], int prev, int next)
{
	if (prev != -1)
	{
		if (dup2(prev, STDIN_FILENO) < 0)
			exit(1);
		close(prev);
	}
	if (next)
	{
		close(fd[0]);
		if (dup2(fd[1], STDOUT_FILENO) < 0)
			exit(1);
		close(fd[1]);
	}
	execvp(cmd[0], cmd);
	exit(1);
}

int	ft_wait(void)
{
	int	status;

	while (wait(&status) != -1)
		;
	return (0);
}

int    picoshell(char **cmds[])
{
	pid_t	pid;
	int		fd[2], prev = -1;

	for (int i = 0; cmds[i]; i++)
	{
		if (cmds[i + 1] && pipe(fd))
			return (1);
		if ((pid = fork()) < 0)
			return (ft_ret(fd, prev, cmds[i + 1] != NULL));
		if (pid == 0)
			ft_child(cmds[i], fd, prev, cmds[i + 1] != NULL);
		if (prev != -1)
			close(prev);
		if (cmds[i + 1])
		{
			close(fd[1]);
			prev = fd[0];
		}
	}
	return (ft_wait());
}