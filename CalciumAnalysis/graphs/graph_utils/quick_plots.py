"""
# draw_plots.py

Module (core): Functions for drawing graphs.

"""
from __future__ import annotations

import logging
from typing import Iterable

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
from matplotlib import rcParams


def set_pub():
    rcParams.update({
        "font.weight"     : "bold",
        "axes.labelweight": 'bold',
        'axes.facecolor'  : 'w',
        "axes.labelsize"  : 15,
        "lines.linewidth" : 0.25,
    })


logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO, format='%(name)s - %(message)s')


class QuickPlot:
    def __init__(
            self,
            data: pd.DataFrame,
            time: Iterable[any],
            input_type: str,
            cmap: str = 'magma',
            **kwargs
            ):

        self.input_type = input_type
        self.cmap = plt.get_cmap(cmap)
        self.data = data
        self.time = time

        self.facecolor = 'white'
        self.color_dict = {
            'ArtSal' : 'dodgerblue',
            'MSG'    : 'darkorange',
            'NaCl'   : 'lime',
            'Sucrose': 'magenta',
            'Citric' : 'yellow',
            'Quinine': 'red',
            'Rinse'  : 'lightsteelblue',
            'Lick'   : 'darkgray'
        }

        self.kwargs = kwargs
        self.checks = {}

    def line_signals(self, ptype: int):
        set_pub()

        for col in self.data.columns:
            fig, ax = plt.subplots(1, 1)
            ax.set_xlabel('Time(s)', weight='bold')
            ax.set_ylabel(f'DF/F ({self.input_type})', weight='bold')
            ax.patch.set_facecolor = 'white'
            Y = self.data.loc[:, col]
            plt.text(0.9, 0.9, f'{col}', transform=ax.transAxes)
            # plt.plot(self.time, Y)
            sns.lineplot(ax=ax,
                         x=self.time,
                         y=Y,
                         sort=False)
            plt.savefig(
                f'/Users/flynnoconnell/Pictures/{ptype}/{ptype}_{col}.png',
                bbox_inches='tight', dpi=600)

    def line_fourier(self, ptype: str):
        for col in self.data.columns:
            plt.figure()
            Y = self.data.loc[:, col]
            # X = self.time

            FFT = np.fft.fft(Y)
            new_N = int(len(FFT) / 2)
            f_nat = 1
            new_X = np.linspace(10 ** -12, f_nat / 2, new_N, endpoint=True)
            new_Xph = 1.0 / new_X
            FFT_abs = np.abs(FFT)
            plt.plot(new_Xph, 2 * FFT_abs[0:int(len(FFT) / 2.)] / len(new_Xph),
                     color='black')
            plt.xlabel('Period ($h$)', fontsize=20)
            plt.ylabel('Amplitude', fontsize=20)
            plt.title('Fourier Transform', fontsize=15)
            plt.grid(True)
            plt.xlim(0, 200)
            plt.show()
