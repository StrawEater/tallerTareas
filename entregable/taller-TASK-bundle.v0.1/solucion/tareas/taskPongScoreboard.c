#include "task_lib.h"

#define WIDTH TASK_VIEWPORT_WIDTH
#define HEIGHT TASK_VIEWPORT_HEIGHT

#define SHARED_SCORE_BASE_VADDR (PAGE_ON_DEMAND_BASE_VADDR + 0xF00)
#define CANT_PONGS 3

#define XBASE 7
#define YBASE 9
#define YGAP 2

void task(void)
{
	screen pantalla;
	// ¿Una tarea debe terminar en nuestro sistema?
	while (true)
	{
		// Completar:
		// - Pueden definir funciones auxiliares para imprimir en pantalla
		// - Pueden usar `task_print`, `task_print_dec`, etc.

		// uint32_t xBase = 10;
		// uint32_t yBase = 10;

		task_print(pantalla, "P1", XBASE + 12, YBASE - 2, C_FG_LIGHT_BLUE);
		task_print(pantalla, "P2", XBASE + 20, YBASE - 2, C_FG_LIGHT_RED);

		for (size_t i = 0; i < CANT_PONGS; i++)
		{
			uint32_t *current_task_record = (uint32_t *)SHARED_SCORE_BASE_VADDR + (i * sizeof(uint32_t) * 2);
			uint32_t puntajeJugador1 = current_task_record[0];
			uint32_t puntajeJugador2 = current_task_record[1];

			task_print(pantalla, "PONG   :", XBASE, YBASE + (i * YGAP), C_FG_CYAN);
			task_print_dec(pantalla, i + 1, 2, XBASE + 5, YBASE + (i * YGAP), C_FG_LIGHT_MAGENTA);
			task_print_dec(pantalla, puntajeJugador1, 6, XBASE + 10, YBASE + (i * YGAP), C_FG_LIGHT_BLUE);
			task_print_dec(pantalla, puntajeJugador2, 6, XBASE + 18, YBASE + (i * YGAP), C_FG_LIGHT_RED);
		}

		syscall_draw(pantalla);
	}
}
