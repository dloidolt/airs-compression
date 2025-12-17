#!/usr/bin/env python3
"""
@file
@author Dominik Loidolt (dominik.loidolt@univie.ac.at)
@date   2025
@copyright GPL-2.0

@brief AIRSPACE CLI Decompression Tests
"""

import unittest

import clitest
from clitest import RETURN_FAILURE, RETURN_SUCCESS, CliTest

DATA_FILE1 = bytes.fromhex("0001 0002")
DATA_FILE2 = bytes.fromhex("0003 0004")


class TestDecompression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cli_test = CliTest()
        cls.airspace = cls.cli_test.run_cli

        # Pre-compute compressed data for common inputs to avoid repeating
        # compression work in every single test.
        result1 = cls.airspace(["-c"], stdin=DATA_FILE1)
        if result1.returncode != RETURN_SUCCESS:
            raise RuntimeError("Failed to compress DATA_FILE1")
        cls.cmp_data1 = result1.stdout

        result2 = cls.airspace(["-c"], stdin=DATA_FILE2)
        if result2.returncode != RETURN_SUCCESS:
            raise RuntimeError("Failed to compress DATA_FILE2")
        cls.cmp_data2 = result2.stdout

    def setUp(self):
        test_name = self.id().split(".")[-1]
        self.test_dir = self.cli_test.change_test_directory(test_name)

    def test_decompress_stdin_to_stdout(self):
        result = self.airspace(stdin=self.cmp_data1)

        self.assertCli(result, stdout_exp=DATA_FILE1)

    def test_decompress_file_to_stdout(self):
        cmp_file = self.test_dir / "compressed_data.air"
        cmp_file.write_bytes(self.cmp_data1)

        result = self.airspace([cmp_file, "--stdout"])

        self.assertCli(result, stdout_exp=DATA_FILE1)

    def test_decompress_file_with_explicit_output_filename(self):
        cmp_file = self.test_dir / "data.air"
        cmp_file.write_bytes(self.cmp_data1)
        decmp_file = self.test_dir / "data.decmp"

        result = self.airspace([cmp_file, "-o", decmp_file])

        self.assertCli(result)
        self.assertEqual(DATA_FILE1, decmp_file.read_bytes(), result.args)

    def test_decompress_auto_removes_air_suffix(self):
        cmp_file = self.test_dir / "data.air"
        cmp_file.write_bytes(self.cmp_data1)
        decmp_file = self.test_dir / "data"

        result = self.airspace([cmp_file])

        self.assertCli(result)
        self.assertEqual(DATA_FILE1, decmp_file.read_bytes(), result.args)

    def test_decompress_auto_removes_ce_suffix(self):
        cmp_file = self.test_dir / "data.ce"
        cmp_file.write_bytes(self.cmp_data1)
        decmp_file = self.test_dir / "data"

        result = self.airspace([cmp_file])

        self.assertCli(result)
        self.assertEqual(DATA_FILE1, decmp_file.read_bytes(), result.args)

    def test_decompress_multiple_files_at_once(self):
        cmp_file1 = self.test_dir / "data1.air"
        cmp_file1.write_bytes(self.cmp_data1)
        cmp_file2 = self.test_dir / "data2.ce"
        cmp_file2.write_bytes(self.cmp_data2)
        out_file1 = self.test_dir / "data1"
        out_file2 = self.test_dir / "data2"

        # Decompress both files
        result = self.airspace([cmp_file1, cmp_file2])
        self.assertCli(result)

        self.assertCli(result)
        self.assertTrue(out_file1.exists(), "Output file 1 should be created")
        self.assertTrue(out_file2.exists(), "Output file 2 should be created")
        self.assertEqual(DATA_FILE1, out_file1.read_bytes(), "Output file 1 mismatch")
        self.assertEqual(DATA_FILE2, out_file2.read_bytes(), "Output file 2 mismatch")

    def test_decompress_unknown_suffix_returns_error(self):
        cmp_file = self.test_dir / "compressed_data.unknown"
        cmp_file.write_bytes(self.cmp_data1)

        result = self.airspace([cmp_file])

        self.assertCli(
            result,
            returncode_exp=RETURN_FAILURE,
            stderr_exp="unknown suffix",
            stderr_match_mode="contains",
        )

    def test_decompress_filename_ending_with_suffix_only_returns_error(self):
        cmp_file = self.test_dir / ".air"
        cmp_file.write_bytes(self.cmp_data1)

        result = self.airspace([cmp_file])

        self.assertCli(
            result,
            returncode_exp=RETURN_FAILURE,
            stderr_exp="is a directory",
            stderr_match_mode="contains",
        )

    def test_decompress_nonexistent_file_returns_error(self):
        nonexistent_file = self.test_dir / "does_not_exist.air"

        result = self.airspace([nonexistent_file])

        self.assertCli(
            result,
            returncode_exp=RETURN_FAILURE,
            stderr_exp="Can't open",
            stderr_match_mode="contains",
        )

    def test_decompress_empty_input_returns_error(self):
        cmp_file = self.test_dir / "empty.air"
        cmp_file.write_bytes(b"")

        result = self.airspace([cmp_file])

        self.assertCli(
            result,
            returncode_exp=RETURN_FAILURE,
            stderr_exp="is empty",
            stderr_match_mode="contains",
        )

    def test_decompress_corrupted_data_returns_error(self):
        cmp_file = self.test_dir / "corrupted.air"
        cmp_file.write_bytes(b"invalid compressed data")

        result = self.airspace([cmp_file])

        self.assertCli(
            result,
            returncode_exp=RETURN_FAILURE,
            stderr_match_mode="contains",
        )

    def test_decompress_to_different_directory(self):
        cmp_file = self.test_dir / "test.air"
        cmp_file.write_bytes(self.cmp_data1)
        out_dir = self.test_dir / "output"
        out_dir.mkdir()
        out_file = out_dir / "test"

        result = self.airspace([cmp_file, "-o", out_file])
        self.assertCli(result)

        self.assertTrue(out_file.exists(), "Output file should be created")
        self.assertEqual(DATA_FILE1, out_file.read_bytes(), "Output content mismatch")


if __name__ == "__main__":
    clitest.main()
