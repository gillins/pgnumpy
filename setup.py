from distutils.core import setup, Extension
import distutils.sysconfig
import numpy
import os
import sys
import glob

main_libdir=distutils.sysconfig.get_python_lib()
pylib_install_subdir = main_libdir.replace(distutils.sysconfig.PREFIX+os.sep,'')

# A few extra include dirs
include_dirs=[numpy.get_include()]
lib_dirs=[]

if os.path.exists('/sw/lib'):
    lib_dirs.append('/sw/lib')

inc_trydirs = ['/usr/include/postgresql',
           '/usr/local/include/postgresql',
           '/usr/local/postgresql/include',
           '/sw/include/postgresql']
for trydir in inc_trydirs:
    if os.path.exists(trydir):
        include_dirs.append(trydir)


sw_altdir='/sw/opt'
if os.path.exists(sw_altdir):
    # look for a postgresql-* directory there
    l=os.listdir(sw_altdir)
    pg_incdirs=[]
    pg_libdirs=[]
    for d in l:
        if d.find('postgresql') != -1:
            pgdir=os.path.join(sw_altdir, d)

            incdir=os.path.join(pgdir, 'include')
            if os.path.exists(incdir):
                pg_incdirs.append(incdir)

            libdir=os.path.join(pgdir, 'lib')
            if os.path.exists(libdir):
                pg_libdirs.append(libdir)

    if len(pg_incdirs) > 0:
        pg_incdirs.sort()
        include_dirs.append(pg_incdirs[-1])

    if len(pg_libdirs) > 0:
        pg_libdirs.sort()
        lib_dirs.append(pg_libdirs[-1])


sources = glob.glob('pgnumpy/*.cc')
depends = ['pgnumpy/cpgnumpy.h']
module1 = Extension('pgnumpy._cpgnumpy', 
                    sources=sources,
                    depends=depends,
                    define_macros=[("NPY_NO_DEPRECATED_API", "NPY_1_7_API_VERSION")],
                    include_dirs=include_dirs,
                    library_dirs=lib_dirs,
                    libraries=['pq'])



if not os.path.exists('ups'):
    os.mkdir('ups')
tablefile=open('ups/pgnumpy.table','w')
tab="""
setupOptional("python")
setupOptional("numpy")
envPrepend(PYTHONPATH,${PRODUCT_DIR}/%s)
""" % pylib_install_subdir 
tablefile.write(tab)
tablefile.close()



setup(name='pgnumpy',
        version='1.0.0',
        description='Numerical python interface to postgresql',
        packages=['pgnumpy'],
        data_files=[('ups',['ups/pgnumpy.table'])],
        ext_modules=[module1]
    )
