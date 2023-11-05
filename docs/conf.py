# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))

import subprocess, os
from pathlib import Path

docs_dir = Path(os.path.dirname(os.path.abspath(__file__)))


def configureDoxyfile(input_dir, output_dir):
    with open(docs_dir / 'Doxyfile.in', 'r') as file:
        filedata = file.read()

    filedata = filedata.replace('@DOXYGEN_INPUT_DIR@', input_dir)
    filedata = filedata.replace('@DOXYGEN_OUTPUT_DIR@', output_dir)

    with open(docs_dir / 'Doxyfile', 'w') as file:
        file.write(filedata)


rtd_build = os.environ.get('READTHEDOCS', None) == 'True'

breathe_projects = {'scnlib': str((docs_dir / 'build/xml').absolute())}
if rtd_build:
    input_dir = docs_dir / '../include'
    output_dir = docs_dir / 'build'
    configureDoxyfile(str(input_dir.absolute()), str(output_dir.absolute()))
    subprocess.call(f'cd {docs_dir}; {docs_dir / "../rtd-downloaded-doxygen/bin/doxygen"}', shell=True)

# -- Project information -----------------------------------------------------

project = 'scnlib'
copyright = '2017, Elias Kosunen'
author = 'Elias Kosunen'

# The full version, including alpha/beta/rc tags
release = '1.1.3'

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = ["breathe", "sphinx.ext.ifconfig"]

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

html_css_files = [
    'css/custom.css'
]

breathe_default_project = "scnlib"
