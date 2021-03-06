# -*- coding: utf-8 -*-
"""
#funcs.py

Module(util): General getter/setter/checker functions.
"""
from __future__ import annotations

import logging
import math
import os
from glob import glob
from pathlib import Path
from typing import Tuple, Iterable, Optional, Sized, Any
import itertools
import numpy as np
import pandas as pd
import scipy.stats as stats

from utils import excepts as e
from misc.wrappers import typecheck

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO, format='%(message)s')


# %% COLLAPSE DATA STRUCTURES

def peek(iterable) -> tuple[Any, itertools.chain] | None:
    try:
        first = next(iterable)
    except StopIteration:
        return None
    return first, itertools.chain([first], iterable)


@typecheck(str)
def check_numeric(my_str: str):
    """ Return boolean True if string is all numbers, otherwise False."""
    return my_str.isdecimal()


def check_path(my_str: str | Path):
    """ Return boolean True if string is a path, otherwise False."""
    return isinstance(my_str, Path) or any(x in my_str for x in ['/', '\\'])


def unzip(val):
    list_of_tuples = list(zip(*val))
    return [list(t) for t in list_of_tuples]


def keys_exist(element, *keys):
    """Check if *keys (nested) exists in `element` (dict)"""
    if len(keys) == 0:
        raise AttributeError('keys_exists() expects at least two arguments, one given.')
    _element = element
    for key in keys:
        try:
            _element = _element[key]
        except KeyError:
            return False
    return True


@typecheck(dict, int)
def iter_events(event_dct, gap: int = 5):
    for event, ts in event_dct.items():
        intervals = interval(ts, gap)
        interv: Iterable
        for interv in intervals:
            yield event, interv


@typecheck(Iterable)
def flatten(lst: Iterable) -> list:
    return [item for sublist in lst for item in sublist]


@typecheck(Iterable, int)
def interval(lst: Iterable[any],
             gap: Optional[int] = 1,
             outer: bool = False
             ) -> list[tuple[Any, Any]]:
    """
    Create intervals where there elements are separated by either:
        -less than gap. 
        -more than gap.

    Args:
        lst (list): Iterable to search.
        gap (int): length of interval.
        outer (bool): Makes larger than (gap) intervals.
    Returns:
         interv (list): New list with created interval.
    """
    interv, tmp = [], []

    for v in lst:
        if not tmp:
            tmp.append(v)

        elif abs(tmp[-1] - v) < gap:
            tmp.append(v)

        elif outer:
            interv.append(tuple((tmp[-1], v)))
            tmp = [v]
        else:
            interv.append(tuple((tmp[0], tmp[-1])))
            tmp = [v]
    return interv


@typecheck(pd.DataFrame, pd.Series, int)
def remove_outliers(df,
                    colors,
                    std: Optional[int] = 2
                    ) -> Tuple[pd.DataFrame, pd.Series]:
    df.reset_index(drop=True, inplace=True)
    colors.reset_index(drop=True, inplace=True)
    ind = (np.abs(stats.zscore(df)) < std).all(axis=1)
    df[ind] = df
    colors[ind] = colors
    assert colors.shape[0] == df.shape[0]

    return df, colors



@typecheck(Iterable[any])
def dup_check(signal: Iterable[any],
              peak_signal: float | int) -> None:
    """
    Check if multiple peak signals are present in taste-response moving window. 

    Args:
        signal (list | np.ndarray): Signal to validate.
        peak_signal (float | int): Largest value in moving window of taste responses.

    Raises:
        exception: DuplicateError.

    Returns:
        None.

    """
    checker = []
    for value in signal:
        dupcheck = math.isclose(value, peak_signal, abs_tol=1.0)
        if dupcheck is True:
            checker.append(dupcheck)
    if not checker:
        raise e.DuplicateError()
    return None


def has_duplicates(to_check: Sized | Iterable[set]):
    """
      Check iterable for duplicates.

      Args:
          to_check (Sized | Iterable[set]): Input iterable to check.

      Returns:
          Bool: Truth value if duplicates are present.

      """
    return len(to_check) != len(set(to_check))


@typecheck(Iterable[any], Iterable[any])
def get_peak_window(time: Iterable[any], peak: float) -> list:
    """
    Returns the index of tracedata centered 1s around the peak flourescent value for that trial.
    Args:
        time (list | pd.Series): List of all time values.
        peak (float) : peak time
    Returns:
         window_ind (list): list of index values to match time.
    """
    time: list

    aux, window_ind = [], []
    for valor in time:
        aux.append(abs(peak - valor))

    window_ind.append(aux.index(min(aux)) - 20)
    window_ind.append(aux.index(min(aux)) + 20)

    return window_ind


def get_matched_time(time: Iterable[any],
                     match: Iterable[any],
                     return_index: Optional[bool] = False) -> list:
    """
    Finds the closest number in tracedata time to the input. Can be a single value, or list.

    :param time: Series
    :param match: list
    :param return_index: bool
    """
    time: pd.Series

    matched_index = []
    matched_time = []
    for t in match:
        temp = []
        for valor in time:
            temp.append(abs(t - valor))
        matched_index.append(temp.index(min(temp)))

    for idx in matched_index:
        this_time = time[idx]
        matched_time.append(this_time)

    if return_index:
        return matched_index
    else:
        return matched_time


# Main wrapper for testing
if __name__ == "__main__":
    pass
