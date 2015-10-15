#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>

#include "libchaos/io.h"
#include "libchaos/mem.h"
#include "libchaos/log.h"
#include "libchaos/gif.h"

#define GIFTEST_WIDTH 100
#define GIFTEST_HEIGHT 100

int giftest_write(void)
{
  struct gif     *gif;
  struct palette *pal;
  struct color    colors[4] = {
    {  0,    0,   0, 0xff },
    { 64,   64,  64, 0xff },
    { 128, 128, 128, 0xff },
    { 192, 192, 192, 0xff }
  };
  uint32_t        x,y,i;
  uint8_t         data;

  gif = gif_new("test.gif", GIF_WRITE);

  pal = gif_palette_make(4, colors);
  
  gif_screen_put(gif, GIFTEST_WIDTH, GIFTEST_HEIGHT, 2, 0, pal);
  
  gif_image_put(gif, 0, 0, GIFTEST_WIDTH, GIFTEST_HEIGHT, 0, pal);
  
  i=0;
  for(y = 0; y < GIFTEST_HEIGHT; y++)
    for(x = 0; x < GIFTEST_WIDTH; x++)
  {
/*    data = i & 0x03;*/
	  data = (y*x*2 + x*x*8) / 80;
	  data &= 0x03;
    gif_data_put(gif, &data, 1);
	i++;
  }

  gif_close(gif);
  gif_save(gif);

  return 0;
}

int main()
{
  printf("log_init\n");

  log_init(STDOUT_FILENO, LOG_ALL, L_status);
  io_init_except(STDOUT_FILENO, STDOUT_FILENO, STDOUT_FILENO);
  mem_init();
  dlink_init();
  gif_init();

  giftest_write();

  gif_shutdown();
  dlink_shutdown();
  mem_shutdown();
  log_shutdown();
  io_shutdown();

  return 0;
}

