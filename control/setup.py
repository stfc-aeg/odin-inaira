"""Setup script for inaira python package."""

import sys
from setuptools import setup, find_packages
import versioneer

with open('requirements.txt') as f:
    required = f.read().splitlines()

setup(name='inaira',
      version=versioneer.get_version(),
      cmdclass=versioneer.get_cmdclass(),
      description='ODIN INAIRA',
      url='https://github.com/stfc-aeg/odin-inaira',
      author='David Symons',
      author_email='david.symons@stfc.ac.uk',
      packages=find_packages('src'),
      package_dir={'': 'src'},
      install_requires=required,
      zip_safe=False,
)
