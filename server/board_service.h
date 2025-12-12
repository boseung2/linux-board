#ifndef BOARD_SERVICE_H
#define BOARD_SERVICE_H

#include "board.h"

// ====== 초기화 함수 ======
void board_system_init(void);

// ====== 게시글 CRUD ======
int board_create_record(const char *author_id,
                        const char *title,
                        const char *content,
                        struct Board *out_post);

int board_get_by_id(int id, struct Board *out_post);

int board_list_range(int offset,
                     int limit,
                     struct Board *out_array,
                     int max_count,
                     int *out_count,
                     const char *search_type,
                     const char *keyword);

int board_update_record(int id,
                        const char *new_title,
                        const char *new_content,
                        const char *user_id);

int board_soft_delete(int id);

int board_load(int id, struct Board *out_post);

#endif
