"""
#file_helpers.py

Module(util): File handling helper functions.
"""
from __future__ import annotations

import logging
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional, Generator, Any, Iterator

import pandas as pd

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO, format='%(message)s')

PROJECT_DIR = Path(__file__).parents[2]
sys.path.append(
    str(PROJECT_DIR / 'apps'))


@dataclass
class FileHandler:
    """
    File handler. From directory, return genorator looping through files.
        Directory structure:
    ./directory
        -| Animal
        --| Date
        ---| Results
        -----| Graphs
        -----| Statistics
        ---| Data_traces*
        ---| Events_gpio (or)
        ---| Events_gpio_processed*
        Use tree() to print current directory tree.
    """

    animal: str = field(repr=False)
    date: str = field(repr=False)
    _directory: Path = field(repr=False)
    _tracename: Optional[str] = 'traces'
    _eventname: Optional[str] = 'processed'
    gpio_file: Optional[bool] = False
    _gpioname: Optional[str] = 'gpio'

    def __post_init__(self):
        self._directory: Path = Path(self._directory)
        self.session: Path = Path(self.animal + self.date)
        self.animaldir: Path = Path(self._directory / self.animal)
        self.sessiondir: Path = Path(self.animaldir / self.date)
        self._make_dirs()

    @property
    def directory(self):
        return self._directory

    @directory.setter
    def directory(self, new_dir: Path):
        self._directory = new_dir

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
        for path in sorted(self.sessiondir.rglob('[!.]*')):  # exclude .files
            depth = len(path.relative_to(self.sessiondir).parts)
            spacer = '    ' * depth
            print(f'{spacer}-|{path.name}')
            return None

    # Generators to iterate each file matching pattern
    def get_traces(self) -> Iterator[str]:
        tracefiles: Generator[Path, None, None] = self.sessiondir.rglob(f'*{self._tracename}*')
        for file in tracefiles:
            yield file

    def get_events(self) -> Iterator[str]:
        eventfiles: Generator[Path, None, None] = self.sessiondir.rglob(f'*{self._eventname}*')
        for file in eventfiles:
            yield file

    def get_gpio_files(self) -> Iterator[str]:
        gpiofile: Generator[Path, None, None] = self.sessiondir.rglob(f'*{self._gpioname}')
        for filepath in gpiofile:
            yield filepath

    # Generators to iterate each matched file and convert to pd.DataFrame 
    def get_tracedata(self) -> Iterator[str]:
        for filepath in self.get_traces():
            tracedata: pd.DataFrame = pd.read_csv(filepath, low_memory=False)
            yield tracedata

    def get_eventdata(self) -> Iterator[str]:
        for filepath in self.get_events():
            eventdata: pd.DataFrame = pd.read_csv(filepath, low_memory=False)
            yield eventdata

    def get_gpiodata(self) -> Iterator[str]:
        for filepath in self.get_gpio_files():
            gpiodata: pd.DataFrame = pd.read_csv(filepath, low_memory=False)
            yield gpiodata

    def unique_path(self, filename):
        counter = 0
        while True:
            counter += 1
            path = self.sessiondir / filename
            if not path.exists():
                return path

    def _make_dirs(self):
        path: Path = self.sessiondir
        path.parents[0].mkdir(parents=True, exist_ok=True)
        return None

    def get_cwd(self):
        return str(self._directory.cwd())

    def get_home_dir(self):
        return str(self._directory.home())
