#!/bin/sh
echo "struct module_import {"
echo "  const char      *name;"
echo "  module_load_t   *load;"
echo "  module_unload_t *unload;"
echo "};"
echo

for dir in msg lclient chanmode usermode service stats
do
  (cd $dir 
   for file in *.c
   do
     base=$(echo $file | sed 's/\.c//')
     echo -e "extern int ${base}_load(struct module *mptr);"
     echo -e "extern void ${base}_unload(struct module *mptr);"
   done)
done

echo
echo "struct module_import module_imports[] = {"

for dir in msg lclient chanmode usermode service stats
do
  (cd $dir 
   for file in *.c
   do
     base=$(echo $file | sed 's/\.c//')
     echo -e "  { \"${base}\", &${base}_load, &${base}_unload },"
   done)
done

echo "};"
