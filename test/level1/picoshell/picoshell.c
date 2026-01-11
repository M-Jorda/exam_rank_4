#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

// Exécute une commande dans le processus enfant
void	exec_cmd(char **cmd, int prev_fd, int pipefd[2], int has_next)
{
	if (prev_fd != -1)
	{
		if (dup2(prev_fd, STDIN_FILENO) == -1)
			exit(1);
		close(prev_fd);
	}
	if (has_next)
	{
		close(pipefd[0]);
		if (dup2(pipefd[1], STDOUT_FILENO) == -1)
			exit(1);
		close(pipefd[1]);
	}
	execvp(cmd[0], cmd);
	exit(1);
}

// Nettoie les file descriptors en cas d'erreur de fork
void	cleanup_on_fork_error(int prev_fd, int pipefd[2], int has_next)
{
	if (has_next)
	{
		close(pipefd[0]);
		close(pipefd[1]);
	}
	if (prev_fd != -1)
		close(prev_fd);
}

// Attend tous les processus enfants et retourne 0 (conforme au sujet)
int	wait_all_children(void)
{
	int	status;

	while (wait(&status) != -1)
		;
	return (0);
}

int picoshell(char **cmds[])
{
	int		i;
	int		pipefd[2];
	int		prev_fd;
	pid_t	pid;

	i = 0;
	prev_fd = -1;
	while (cmds[i])
	{
		if (cmds[i + 1] && pipe(pipefd))
			return (1);
		pid = fork();
		if (pid == -1)
		{
			cleanup_on_fork_error(prev_fd, pipefd, cmds[i + 1] != NULL);
			return (1);
		}
		if (pid == 0)
			exec_cmd(cmds[i], prev_fd, pipefd, cmds[i + 1] != NULL);
		if (prev_fd != -1)
			close(prev_fd);
		if (cmds[i + 1])
		{
			close(pipefd[1]);
			prev_fd = pipefd[0];
		}
		i++;
	}
	return (wait_all_children());
}
