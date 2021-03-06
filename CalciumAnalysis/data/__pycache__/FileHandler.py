"""
#file_helpers.py

Module(util): File handling helper functions.
"""
from __future__ import annotations
from typing import Optional
import os
import pandas as pd
import logging
from pathlib import Path
from glob import glob

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO, format='%(message)s')


class FileHandler(object):
    """
    File handler.
    Directory structure: 
        
    ./directory
        -| Animal 
        --| Date
        ---| Results
        -----| Graphs
        -----| Statistics
        ---| Data_traces*
        ---| Data_gpio_processed*
        Use tree() to print current directory tree.
        
    Args:
        directory (str): Path to directory containing data.
        animal (str): Animal name.
        date (str): Current session date  
        tracename (str): String segment of trace to fetch.
        eventname (str): String segment of event file to fetch.
            -String used in unix-style pattern-matching, surrounded by wildcards:
                e.g. *traces*, *events*, *processed*, *GPIO*
            -Use these args to match string in file for processing.
    """

    def __init__(self,
                 directory: str,
                 animal: str,
                 date: str,
                 tracename: Optional[str] = 'traces',
                 eventname: Optional[str] = 'processed'):
        self.animal = animal
        self.date = date
        self._directory = Path(directory)
        self.session = self.animal + '_' + self.date
        self.animaldir = self._directory / self.animal
        self.sessiondir = self.animaldir / self.date
        self._tracename = tracename
        self._eventname = eventname
        self._make_dirs()

    @property
    def directory(self):
        return self._directory

    @directory.setter
    def directory(self, new_dir: str):
        self._directory = Path(new_dir)

    @property
    def tracename(self):
        return self._tracename

    @tracename.setter
    def tracename(self, new_tracename):
        self._tracename = new_tracename

    @property
    def eventname(self):
        return self._eventname

    @eventname.setter
    def eventname(self, new_eventname):
        self._eventname = new_eventname

    def tree(self):
        print(f'-|{self._directory}')
        for path in sorted(self.directory.rglob('[!.]*')):  # exclude .files
            depth = len(path.relative_to(self.directory).parts)
            spacer = '    ' * depth
            print(f'{spacer}-|{path.name}')
            return None

    def get_traces(self):
        tracefiles = self.sessiondir.rglob(f'*{self.tracename}*')
        for file in tracefiles:
            yield file

    def get_events(self):
        eventfiles = self.sessiondir.rglob(f'*{self.eventname}*')
        for file in eventfiles:
            yield file

    def get_tracedata(self):
        for filepath in self.get_traces():
            tracedata = pd.read_csv(filepath, low_memory=False)
            yield tracedata

    def get_eventdata(self):
        for filepath in self.get_events():
            eventdata = pd.read_csv(filepath, low_memory=False)
            yield eventdata

    def unique_path(self, filename):
        counter = 0
        while True:
            counter += 1
            path = self.sessiondir / filename
            if not path.exists():
                return path

    def _make_dirs(self):
        path = self.sessiondir
        Path(path).parents[0].mkdir(parents=True, exist_ok=True)
        return None


datadir = '/Users/flynnoconnell/Documents/Work/Data'
animal = 'PGT13'
date = '121021'

filehandler = FileHandler(datadir, animal, date)

traces = [file for file in filehandler.get_tracedata()]
events = [file for file in filehandler.get_eventdata()]

# def get_dir(data_dir: str,
#             _id: str,
#             date: str,
#             pick: int
#             ):
#     os.chdir(data_dir)
#     datapath = Path(data_dir) / _id / date

#     files = (glob(os.path.join(datapath, '*traces*')))
#     logging.info(f'{len(files)} trace files found:')
#     for file in (files):
#         file = Path(file)
#         logging.info(f'{file.stem}')
#         logging.info('-' * 15)

#     if len(files) > 1:
#         if pick == 0:
#             tracepath = Path(files[0])
#             logging.info('Taking trace file: {}'.format(tracepath.name))
#         else:
#             tracepath = Path(files[pick])
#             logging.info('Taking trace file: {}'.format(tracepath.name))
#     elif len(files) == 1:
#         tracepath = Path(files[0])
#     else:
#         logging.info(f'Files for {_id}, {date} not found.')
#         raise FileNotFoundError

#     eventpath = Path(glob(os.path.join(datapath, '*processed*'))[0])
#     tracedata = pd.read_csv(tracepath, low_memory=False)
#     eventdata = pd.read_csv(eventpath, low_memory=False)

#     return tracedata, eventdata
