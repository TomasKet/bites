#ifndef __STORAS__H__
#define __STORAS__H__

int init_storas(void);
int restore_defaults(void);
int storas_set_str(const char* key, const char* value);
int storas_set_int(const char* key, int value);
int storas_get_int(const char* key, int* out_value);

extern char ssid[];
extern char pass[];
extern char stream_uri_custom[];

#endif /* __STORAS__H__ */
