#ifndef RENDERER_H
#define RENDERER_H

typedef struct Renderer Renderer;

Renderer* renderer_new(void);

void renderer_run(Renderer* renderer);

#endif
