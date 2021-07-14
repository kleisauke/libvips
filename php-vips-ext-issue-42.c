// compile with
// clang -g -Wall php-vips-ext-issue-42.c `pkg-config vips --cflags --libs` -o php-vips-ext-issue-42
// run with
// ./php-vips-ext-issue-42
#include <sys/wait.h>
#include <vips/vips.h>

int main(int argc, char **argv) {
  pid_t pid;
  VipsImage *in;

  // Disable buffering on stdout
  setbuf(stdout, NULL);

  printf("vips_init\n");
  if (VIPS_INIT(argv[0]))
    vips_error_exit("unable to start VIPS");

  printf("fork\n");
  pid = fork();

  switch (pid) {
    case -1:
      vips_error_exit("fork failed with: %s\n", strerror(errno));

    case 0:
      printf("PID %d (child) doing work\n", getpid());
      if (vips_black(&in, 100, 100, NULL))
        vips_error_exit("vips_black failed\n");

      if (vips_image_write_to_file(in, "x.jpg", NULL))
        vips_error_exit("vips_image_write_to_file failed\n");

      g_object_unref(in);
      printf("child done\n");
      break;

    default:
      printf("PID %d (parent) waiting for child PID %d\n", getpid(), pid);
      waitpid(pid, NULL, 0);

      printf("exit\n");
      exit(0);
  }

  return 0;
}
