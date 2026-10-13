#ifndef NS_MOCK_SDK_H
#define NS_MOCK_SDK_H
/* Independently authored contract shim, NOT an SDK or ABI verification. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define UNUSED(x) ((void)(x))
#define RECORD_STORAGE "storage"
#define RECORD_GUI "gui"
#define FuriWaitForever UINT32_MAX
typedef enum { FuriStatusOk, FuriStatusError } FuriStatus;
typedef enum { FuriMutexTypeNormal } FuriMutexType;
typedef enum { InputKeyUp, InputKeyDown, InputKeyLeft, InputKeyRight, InputKeyOk, InputKeyBack } InputKey;
typedef enum { InputTypeShort, InputTypeRepeat, InputTypePress, InputTypeLong } InputType;
typedef struct { InputKey key; InputType type; } InputEvent;
typedef struct FuriMutex FuriMutex;
typedef struct FuriMessageQueue FuriMessageQueue;
typedef struct ViewPort ViewPort;
typedef struct Gui Gui;
typedef struct Canvas Canvas;
typedef enum { FontSecondary } Font;
typedef enum { GuiLayerFullscreen } GuiLayer;
typedef enum { FSE_OK, FSE_EXIST, FSE_NOT_EXIST, FSE_ERROR } FS_Error;
typedef enum { FSAM_READ, FSAM_WRITE } FS_AccessMode;
typedef enum { FSOM_OPEN_EXISTING, FSOM_CREATE_ALWAYS, FSOM_CREATE_NEW } FS_OpenMode;
typedef struct Storage Storage;
typedef struct File File;
FuriMutex* furi_mutex_alloc(FuriMutexType);
void furi_mutex_free(FuriMutex*);
FuriStatus furi_mutex_acquire(FuriMutex*, uint32_t);
void furi_mutex_release(FuriMutex*);
FuriMessageQueue* furi_message_queue_alloc(size_t, size_t);
void furi_message_queue_free(FuriMessageQueue*);
FuriStatus furi_message_queue_put(FuriMessageQueue*, const void*, uint32_t);
FuriStatus furi_message_queue_get(FuriMessageQueue*, void*, uint32_t);
void* furi_record_open(const char*);
void furi_record_close(const char*);
uint32_t furi_get_tick(void);
uint32_t furi_ms_to_ticks(uint32_t);
ViewPort* view_port_alloc(void);
void view_port_free(ViewPort*);
void view_port_draw_callback_set(ViewPort*, void (*)(Canvas*, void*), void*);
void view_port_input_callback_set(ViewPort*, void (*)(InputEvent*, void*), void*);
void view_port_update(ViewPort*);
void view_port_enabled_set(ViewPort*, bool);
void gui_add_view_port(Gui*, ViewPort*, GuiLayer);
void gui_remove_view_port(Gui*, ViewPort*);
void canvas_clear(Canvas*);
void canvas_set_font(Canvas*, Font);
void canvas_draw_str(Canvas*, int, int, const char*);
uint16_t canvas_string_width(Canvas*, const char*);
void canvas_draw_line(Canvas*, int, int, int, int);
File* storage_file_alloc(Storage*);
void storage_file_free(File*);
bool storage_file_open(File*, const char*, FS_AccessMode, FS_OpenMode);
bool storage_file_close(File*);
size_t storage_file_read(File*, void*, size_t);
size_t storage_file_write(File*, const void*, size_t);
uint64_t storage_file_size(File*);
bool storage_file_sync(File*);
FS_Error storage_file_get_error(File*);
FS_Error storage_common_mkdir(Storage*, const char*);
void mock_sdk_reset(void);
void mock_sdk_events(const InputEvent*, size_t);
void mock_sdk_fail_allocation(unsigned);
void mock_sdk_fail_storage(bool);
void mock_sdk_fail_record(const char*);
void mock_sdk_fail_mutex(unsigned);
void mock_sdk_fail_queue(bool);
void mock_sdk_burst(bool);
unsigned mock_sdk_queue_drops(void);
unsigned mock_sdk_live(void);
unsigned mock_sdk_records(void);
unsigned mock_sdk_draws(void);
#endif
