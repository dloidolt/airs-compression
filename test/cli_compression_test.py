#!/usr/bin/env python3
"""
@file   cli_compression_test.py
@author Dominik Loidolt (dominik.loidolt@univie.ac.at)
@date   2025

@copyright GPLv2
This program is free software; you can redistribute it and/or modify it
under the terms and conditions of the GNU General Public License,
version 2, as published by the Free Software Foundation.

This program is distributed in the hope it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
more details.

@brief AIRSPACE CLI Compression Tests
"""

import os
import unittest
from pathlib import Path
from typing import Callable, List, Optional, Union

import clitest
from clitest import RETURN_FAILURE, CliTest, assertCli

CMP_HDR_SIZE = 8
DATA_FILE1 = bytes.fromhex("0001 0002")
DATA_FILE2 = bytes.fromhex("0003 0004")
EXPECTED_COMPRESSED_DATA = DATA_FILE1 + DATA_FILE2


class TestCompression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cli_test = CliTest()
        cls.airspace = cls.cli_test.run_cli

    def setUp(self):
        test_name = self.id().split(".")[-1]
        self.test_dir = self.cli_test.change_test_directory(test_name)
        self.file1 = self.test_dir / "file_1.bin"
        self.file2 = self.test_dir / "file_2.bin"
        self.file1.write_bytes(DATA_FILE1)
        self.file2.write_bytes(DATA_FILE2)

    def test_compress_2_files_to_dev_null(self):
        result = self.airspace(
            ["-c", self.file1, self.file2, "-o", os.devnull, "--quiet"]
        )

        self.assertCli(result)

    def test_compress_2_files_to_stdout(self):
        result = self.airspace(["-c", self.file1, self.file2, "--quiet"])

        self.assertCli(
            result,
            stdout_exp=EXPECTED_COMPRESSED_DATA,
            stdout_match_mode="contains",
        )

    def test_compress_2_files_to_output_file(self):
        cmp_file = self.test_dir / "output.air"

        result = self.airspace(
            ["-c", self.file1, self.file2, "-o", cmp_file, "--quiet"]
        )

        self.assertCli(result)
        self.assertTrue(cmp_file.exists())
        cmp_data = cmp_file.read_bytes()[CMP_HDR_SIZE:]
        self.assertEqual(EXPECTED_COMPRESSED_DATA, cmp_data, result.args)

    def test_compress_data_from_stdin(self):
        for arg in [["-"], []]:
            with self.subTest(arg=arg):
                cmp_file = self.test_dir / (str(arg) + "output.air")
                self.assertFalse(cmp_file.exists())

                result = self.airspace(
                    arg + ["-c", "-o", cmp_file, "--quiet"],
                    stdin=DATA_FILE1,
                )

                self.assertCli(result)
                self.assertTrue(cmp_file.exists())
                cmp_data = cmp_file.read_bytes()[CMP_HDR_SIZE:]
                self.assertEqual(DATA_FILE1, cmp_data, result.args)

    def test_files_to_decompress_must_have_same_size(self):
        to_small_file = self.test_dir / "to_small_file.bin"
        to_small_file.write_bytes(bytes.fromhex("0003"))

        result = self.airspace(["-c", self.file1, to_small_file, "--quiet"])

        self.assertCli(
            result,
            returncode_exp=RETURN_FAILURE,
            stderr_exp="same size",
            stderr_match_mode="contains",
        )

    def test_not_overwrite_existing_file(self):
        existing_file = self.test_dir / "existing_file.txt"
        existing_file.write_text("Do not overwrite this file!")

        result = self.airspace(
            ["-c", self.file1, self.file2, "-o", existing_file, "--quiet"]
        )

        self.assertCli(
            result,
            returncode_exp=RETURN_FAILURE,
            stderr_exp="already exists",
            stderr_match_mode="contains",
        )

    def test_not_overwrite_existing_directory(self):
        existing_dir = self.test_dir / "existing_dir"
        existing_dir.mkdir()

        result = self.airspace(
            ["-c", self.file1, self.file2, "-o", existing_dir, "--quiet"]
        )

        self.assertCli(
            result,
            returncode_exp=RETURN_FAILURE,
            stderr_exp="already exists",
            stderr_match_mode="contains",
        )


if __name__ == "__main__":
    clitest.main()
