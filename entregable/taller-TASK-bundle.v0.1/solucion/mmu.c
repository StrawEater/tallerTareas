/* ** por compatibilidad se omiten tildes **
================================================================================
 TRABAJO PRACTICO 3 - System Programming - ORGANIZACION DE COMPUTADOR II - FCEN
================================================================================

  Definicion de funciones del manejador de memoria
*/

#include "mmu.h"
#include "i386.h"

#include "kassert.h"

static pd_entry_t *kpd = (pd_entry_t *)KERNEL_PAGE_DIR;
static pt_entry_t *kpt = (pt_entry_t *)KERNEL_PAGE_TABLE_0;

static const uint32_t identity_mapping_end = 0x003FFFFF;
static const uint32_t user_memory_pool_end = 0x02FFFFFF;

static paddr_t next_free_kernel_page = 0x100000;
static paddr_t next_free_user_page = 0x400000;

/**
 * kmemset asigna el valor c a un rango de memoria interpretado
 * como un rango de bytes de largo n que comienza en s
 * @param s es el puntero al comienzo del rango de memoria
 * @param c es el valor a asignar en cada byte de s[0..n-1]
 * @param n es el tamaño en bytes a asignar
 * @return devuelve el puntero al rango modificado (alias de s)
 */
static inline void *kmemset(void *s, int c, size_t n)
{
  uint8_t *dst = (uint8_t *)s;
  for (size_t i = 0; i < n; i++)
  {
    dst[i] = c;
  }
  return dst;
}

/**
 * zero_page limpia el contenido de una página que comienza en addr
 * @param addr es la dirección del comienzo de la página a limpiar
 */
static inline void zero_page(paddr_t addr)
{
  kmemset((void *)addr, 0x00, PAGE_SIZE);
}

void mmu_init(void) {}

/**
 * mmu_next_free_kernel_page devuelve la dirección física de la próxima página de kernel disponible.
 * Las páginas se obtienen en forma incremental, siendo la primera: next_free_kernel_page
 * @return devuelve la dirección de memoria de comienzo de la próxima página libre de kernel
 */
paddr_t mmu_next_free_kernel_page(void)
{
  paddr_t res = next_free_kernel_page;
  next_free_kernel_page += 0x1000;
  return res;
}

/**
 * mmu_next_free_user_page devuelve la dirección de la próxima página de usuarix disponible
 * @return devuelve la dirección de memoria de comienzo de la próxima página libre de usuarix
 */
paddr_t mmu_next_free_user_page(void)
{
  paddr_t res = next_free_user_page;
  next_free_user_page += 0x1000;
  return res;
}

/**
 * mmu_init_kernel_dir inicializa las estructuras de paginación vinculadas al kernel y
 * realiza el identity mapping
 * @return devuelve la dirección de memoria de la página donde se encuentra el directorio
 * de páginas usado por el kernel
 */
paddr_t mmu_init_kernel_dir(void)
{

  pd_entry_t primeraEntrada;
  primeraEntrada.pt = KERNEL_PAGE_TABLE_0 >> 12;
  primeraEntrada.attrs = MMU_W | MMU_P;
  kpd[0] = primeraEntrada;
  for (size_t index = 0; index < 1024; index++)
  {
    pt_entry_t entradaMapeoDirecto;
    entradaMapeoDirecto.page = index;
    entradaMapeoDirecto.attrs = MMU_W | MMU_P;
    kpt[index] = entradaMapeoDirecto;
  }

  return KERNEL_PAGE_DIR;
}

/**
 * mmu_map_page agrega las entradas necesarias a las estructuras de paginación de modo de que
 * la dirección virtual virt se traduzca en la dirección física phy con los atributos definidos en attrs
 * @param cr3 el contenido que se ha de cargar en un registro CR3 al realizar la traducción
 * @param virt la dirección virtual que se ha de traducir en phy
 * @param phy la dirección física que debe ser accedida (dirección de destino)
 * @param attrs los atributos a asignar en la entrada de la tabla de páginas
 */
void mmu_map_page(uint32_t cr3, vaddr_t virt, paddr_t phy, uint32_t attrs)
{

  // Tomamos la direccion del cr3 como el inicio de un directorio de paginas
  pd_entry_t *directorio = (pd_entry_t *)(CR3_TO_PAGE_DIR(cr3));

  // Desmenuzamos a la direccion virtual en cada uno de sus significados individuales
  uint32_t indexDirectorio = VIRT_PAGE_DIR(virt);
  uint32_t indexTabla = VIRT_PAGE_TABLE(virt);

  // Chequeamos si existe la entrada del directorio de paginas que busca la direccion virtual
  pd_entry_t *entradaDirectorio = &(directorio[indexDirectorio]);
  if ((entradaDirectorio->attrs & MMU_P) == 0)
  {
    // Si no existe, creamos la tabla
    entradaDirectorio->pt = mmu_next_free_kernel_page() >> 12;
    entradaDirectorio->attrs = attrs | MMU_P;
  }

  entradaDirectorio->attrs = entradaDirectorio->attrs | attrs;

  // tomamos la direccion de la entrada de directorio como el inicio de una tabla de paginas
  pt_entry_t *tabla = (pt_entry_t *)(MMU_ENTRY_PADDR(entradaDirectorio->pt));

  // guardamos la pagina que indica la direccion virtual
  pt_entry_t *entradaTabla = &tabla[indexTabla];

  // guardamos la nueva direccion fisica y atributos dentro de la entrada indicada
  entradaTabla->page = phy >> 12;
  entradaTabla->attrs = attrs | MMU_P;

  tlbflush();
}

/**
 * mmu_unmap_page elimina la entrada vinculada a la dirección virt en la tabla de páginas correspondiente
 * @param virt la dirección virtual que se ha de desvincular
 * @return la dirección física de la página desvinculada
 */
paddr_t mmu_unmap_page(uint32_t cr3, vaddr_t virt)
{

  // Tomamos la direccion del cr3 como el inicio de un directorio de paginas
  pd_entry_t *directorio = (pd_entry_t *)(CR3_TO_PAGE_DIR(cr3));

  // Desmenuzamos a la direccion virtual en cada uno de sus significados individuales
  uint32_t indexDirectorio = VIRT_PAGE_DIR(virt);
  uint32_t indexTabla = VIRT_PAGE_TABLE(virt);

  // Chequeamos si existe la entrada del directorio de paginas que busca la direccion virtual
  pd_entry_t *entradaDirectorio = &directorio[indexDirectorio];

  // tomamos la direccion de la entrada de directorio como el inicio de una tabla de paginas
  pt_entry_t *tabla = (pt_entry_t *)(MMU_ENTRY_PADDR(entradaDirectorio->pt));

  // guardamos la pagina que indica la direccion virtual
  pt_entry_t *entradaTabla = &tabla[indexTabla];
  entradaTabla->attrs = 0;
  uint32_t addresFisica = MMU_ENTRY_PADDR(entradaTabla->page);

  tlbflush();

  return addresFisica;
}

#define DST_VIRT_PAGE 0xA00000
#define SRC_VIRT_PAGE 0xB00000

/**
 * copy_page copia el contenido de la página física localizada en la dirección src_addr a la página física ubicada en dst_addr
 * @param dst_addr la dirección a cuya página queremos copiar el contenido
 * @param src_addr la dirección de la página cuyo contenido queremos copiar
 *
 * Esta función mapea ambas páginas a las direcciones SRC_VIRT_PAGE y DST_VIRT_PAGE, respectivamente, realiza
 * la copia y luego desmapea las páginas. Usar la función rcr3 definida en i386.h para obtener el cr3 actual
 */
void copy_page(paddr_t dst_addr, paddr_t src_addr)
{

  uint32_t cr3 = rcr3();
  // mapear dst_addr a DST_VIRT_PAGE;
  mmu_map_page(cr3, DST_VIRT_PAGE, dst_addr, MMU_P | MMU_W);

  // mapear src_addr a SRC_VIRT_PAGE;
  mmu_map_page(cr3, SRC_VIRT_PAGE, src_addr, MMU_P | MMU_W);

  for (size_t i = 0; i < 64; i++)
  {
    uint64_t *dataACopiar = (uint64_t *)(SRC_VIRT_PAGE + 8 * i);
    uint64_t *destino = (uint64_t *)(DST_VIRT_PAGE + 8 * i);
    *destino = *dataACopiar;
  }

  mmu_unmap_page(cr3, SRC_VIRT_PAGE);
  mmu_unmap_page(cr3, DST_VIRT_PAGE);
}

void pruebaCopy(void)
{
  kmemset((void *)0x3FA000, 1, PAGE_SIZE);
  copy_page(VIDEO, 0x3FA000);
}

/**
 * mmu_init_task_dir inicializa las estructuras de paginación vinculadas a una tarea cuyo código se encuentra en la dirección phy_start
 * @pararm phy_start es la dirección donde comienzan las dos páginas de código de la tarea asociada a esta llamada
 * @return el contenido que se ha de cargar en un registro CR3 para la tarea asociada a esta llamada
 */
paddr_t mmu_init_task_dir(paddr_t phy_start)
{
  // Pedimos espacio en el area de kernel para las nuevas estructuras de paginacion
  paddr_t directorioTablasTareaAddres = mmu_next_free_kernel_page();

  // Creamos la pagina con el mapeo directo del kernel

  for (size_t index = 0; index < 1024; index++)
  {
    mmu_map_page(directorioTablasTareaAddres, index * PAGE_SIZE, index * PAGE_SIZE, MMU_P | MMU_W);
  }

  // Creamos las 2 paginas para el mapeo del codigo
  mmu_map_page(directorioTablasTareaAddres, TASK_CODE_VIRTUAL, phy_start, MMU_P | MMU_U);
  mmu_map_page(directorioTablasTareaAddres, TASK_CODE_VIRTUAL + PAGE_SIZE, phy_start + PAGE_SIZE, MMU_P | MMU_U);

  // Creamos la pagina para la pila
  paddr_t direccionFisicaPila = mmu_next_free_user_page();
  mmu_map_page(directorioTablasTareaAddres, TASK_STACK_BASE - PAGE_SIZE, direccionFisicaPila, MMU_P | MMU_U | MMU_W);

  // Creamos la pagina para la memoria compartida entre tareas
  mmu_map_page(directorioTablasTareaAddres, TASK_SHARED_PAGE, SHARED, MMU_P | MMU_U);

  // Devolvemos el cr3 apuntando al directorio de la tarea
  return directorioTablasTareaAddres;
}

// COMPLETAR: devuelve true si se atendió el page fault y puede continuar la ejecución
// y false si no se pudo atender
bool page_fault_handler(vaddr_t virt)
{
  print("Atendiendo page fault...", 0, 0, C_FG_WHITE | C_BG_BLACK);
  // Chequeemos si el acceso fue dentro del area on-demand
  if ((virt >= ON_DEMAND_MEM_START_VIRTUAL) && (virt <= ON_DEMAND_MEM_END_VIRTUAL))
  {
    // En caso de que si, mapear la pagina
    uint32_t cr3 = rcr3();
    mmu_map_page(cr3, ON_DEMAND_MEM_START_VIRTUAL, ON_DEMAND_MEM_START_PHYSICAL, MMU_P | MMU_U | MMU_W);

    return true;
  }
  return false;
}
