#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define READ_END 0
#define WRITE_END 1

typedef struct s_cmd {
	char	**argv;
	int		argc;
	int		pipe_fd[2];
	int		prev_pipe_fd;
	bool	is_pipe;
	bool	is_prev_pipe;
} t_cmd;

void	ft_puterr(char *str) {
	int	i = 0;
	while (str[i]) {
		write(2, &str[i], 1);
		i++;
	}
}

void	fatal() {
	ft_puterr("error: fatal\n");
	exit(1);
}

int	init_cmd(t_cmd *cmd, char **argv, int argc, int i) {
	int start = i;
	while (i < argc && strcmp(argv[i], "|") != 0 && strcmp(argv[i], ";") != 0) {
		i++;
	}
	int len = i - start;
	if (len > 0) {
		cmd->argc = len;
		cmd->argv = argv + start;
		cmd->is_prev_pipe = cmd->is_pipe;
		cmd->is_pipe = i < argc && strcmp(argv[i], "|") == 0;
		cmd->prev_pipe_fd = cmd->pipe_fd[READ_END];
		argv[i] = NULL;
	}
	return len;
}

void	cd(t_cmd *cmd) {
	if (cmd->argc != 2)
		ft_puterr("error: cd: bad arguments\n");
	if (chdir(cmd->argv[1])) {
		ft_puterr("error: cd: cannot change directory to ");
		ft_puterr(cmd->argv[1]);
		ft_puterr("\n");
	}
}

void	exec(t_cmd *cmd, char **env) {
	if (cmd->is_pipe && pipe(cmd->pipe_fd))
		fatal();
	
	pid_t cpid = fork();
	if (cpid == -1)
		fatal();
	if (cpid == 0) {
		if (cmd->is_pipe && (dup2(cmd->pipe_fd[WRITE_END], STDOUT_FILENO) < 0
						|| close(cmd->pipe_fd[READ_END])))
			fatal();
		if (cmd->is_prev_pipe && dup2(cmd->prev_pipe_fd, STDIN_FILENO) < 0)
			fatal();

		execve(cmd->argv[0], cmd->argv, env);
		ft_puterr("error: cannot execute ");
		ft_puterr(cmd->argv[0]);
		ft_puterr("\n");
		exit(1);
	} else {
		if (cmd->is_pipe && close(cmd->pipe_fd[WRITE_END]))
			fatal();
		if (cmd->is_prev_pipe && close(cmd->prev_pipe_fd))
			fatal();
		waitpid(cpid, 0, 0);
	}
}

int	main(int argc, char **argv, char **env) {
	int	i = 0;
	t_cmd	cmd;
	cmd.is_pipe = false;
	while (++i < argc) {
		int start = i;
		i += init_cmd(&cmd, argv, argc, i);
		if (i > start) {
			if(strcmp(cmd.argv[0], "cd") == 0) {
				cd(&cmd);
			} else {
				exec(&cmd, env);
			}
		}
	}
	return 0;
}