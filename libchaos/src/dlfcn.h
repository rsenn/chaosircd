/* dlfcn.h */
#ifndef DLFCN_H_
#define DLFCN_H_

/* declarations used for dynamic linking support routines */
extern void *dlopen (const char *, int);
extern void *dlsym  (void *, const char *);
extern int   dlclose(void *);
extern char *dlerror(void);

#define RTLD_DEFAULT 0

/* values for mode argument to dlopen */
#define RTLD_LOCAL  0 /* symbols in this dlopen'ed obj are not visible to other dlopen'ed objs */
#define RTLD_LAZY   1 /* lazy function call binding */
#define RTLD_NOW    2 /* immediate function call binding */
#define RTLD_GLOBAL 4 /* symbols in this dlopen'ed obj are visible to other dlopen'ed objs */

#endif /* DLFCN_H_ */