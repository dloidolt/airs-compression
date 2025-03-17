#!/usr/bin/env python3
"""
@file
@author Dominik Loidolt (dominik.loidolt@univie.ac.at)
@date   2025
@copyright GPL-2.0

@brief Makes testing command line interfaces (CLI) easy
"""

import argparse
import os
import shlex
import shutil
import subprocess
import sys
import unittest
from pathlib import Path
from typing import List, Optional, Union

SCRATCH_DIR_NAME = "scratch"

RETURN_SUCCESS = 0
RETURN_FAILURE = 1


class CliTest:
    """
    Base class for CLI tests, providing common functionality for setting up
    test environments and running CLI commands.
    """

    cli_path: Optional[Path] = None
    test_root: Path = Path.cwd()
    cli_cwd = None
    # preserve_test_artifacts: bool = False

    def run_cli(
        self, args: Union[None, str, List[str]] = None, stdin=None
    ) -> subprocess.CompletedProcess:
        """
            Execute the CLI executable with specified arguments.

        Args:
            args: Command-line arguments as a string, list of strings, or None.
            stdin: Input to be passed to the command's standard input.

        Returns:
            A CompletedProcess object containing execution results.
        """
        if self.cli_path is None:
            raise FileNotFoundError("No CLI executable specified")

        if not self.cli_path.exists():
            raise FileNotFoundError(f"CLI executable not found at {self.cli_path}")

        if not self.cli_path.is_file() or not os.access(self.cli_path, os.X_OK):
            raise PermissionError(
                f"CLI executable at {self.cli_path} is not executable"
            )

        if isinstance(args, str):
            args = shlex.split(args)
        elif args is None:
            args = []

        return subprocess.run(
            [str(self.cli_path)] + args,
            input=stdin,
            capture_output=True,
            text=False,
            check=False,
            cwd=self.cli_cwd,
        )

    def change_test_directory(self, name) -> Path:
        """
        Creates a test case directory within the scratch directory and changes into it.

        This method:
            - Generates a directory name based on the provided name inside the scratch directory.
            - Removes any existing directory with the same name.
            - Sets the current working directory for run_cli to this new directory.

        Args:
            name: A string representing the name of the directory to create inside the scratch
                  directory.

        Returns:
            Path: A fresh, empty directory path for the current test case.
        """
        scratch_dir = self.test_root / SCRATCH_DIR_NAME
        self.cli_cwd = create_fresh_directory(scratch_dir / name)
        return self.cli_cwd


def assertCli(
    self,
    result: subprocess.CompletedProcess,
    stdout_exp: Union[str, bytes] = "",
    stderr_exp: Union[str, bytes] = "",
    returncode_exp: int = RETURN_SUCCESS,
    stdout_match_mode: str = "exact",
    stderr_match_mode: str = "exact",
) -> None:
    """
    Assert command execution results match expected values.

    Args:
    result: The completed process result from running a CLI command.
        stdout_exp: Expected stdout output.
        stderr_exp: Expected stderr output.
        returncode_exp: Expected return code.
        stdout_match_mode: {'exact', 'contains'} Matching strategy for standard output.
            - 'exact': Requires full match of output.
            - 'contains': Checks if expected output is present in actual output.
        stderr_match_mode: {'exact', 'contains'} Matching strategy for standard error.
            - 'exact': Requires full match of output.
            - 'contains': Checks if expected output is present in actual output.

    Examples:
        >>> test_result = run_cli(["--version"])
        >>> self.assertCli(
        ...     test_result,
        ...     stdout_exp="1.0.0",
        ...     returncode_exp=0
        ... )

        >>> self.assertCli(
        ...     test_result,
        ...     stderr_exp="Error: Invalid option",
        ...     stdout_match_mode="contains",
        ...     returncode_exp=1
        ... )
    """

    def path_to_string(path: Path) -> str:
        return str(path.relative_to(Path.cwd()))

    def create_command_string(arguments) -> str:
        if not isinstance(arguments, list):
            arguments = [arguments]  # ensure list

        arguments_converted = []
        for arg in arguments:
            if isinstance(arg, Path):
                arguments_converted.append(path_to_string(arg))
            else:
                arguments_converted.append(arg)

        return shlex.join(arguments_converted) + "\n"

    error_context = "Command:\n" + create_command_string(result.args)

    if isinstance(result.stdout, bytes) and not isinstance(stdout_exp, bytes):
        stdout_exp = stdout_exp.encode()
    if isinstance(result.stderr, bytes) and not isinstance(stderr_exp, bytes):
        stderr_exp = stderr_exp.encode()

    if stderr_match_mode == "exact":
        stderr_assertion = self.assertEqual
    elif stderr_match_mode == "contains":
        stderr_assertion = self.assertIn
    elif stderr_match_mode == "ignore":

        def stderr_assertion(*args, **kwargs):
            pass  # No action for 'ignore' mode

    else:
        raise ValueError(f"Invalid stderr_match_mode: {stderr_match_mode}")

    stderr_assertion(
        stderr_exp,
        result.stderr,
        f"stderr mismatch\n{error_context}"
        f"Expected: {repr(stderr_exp)} ({stderr_match_mode})\n"
        f"Actual:   {repr(result.stderr)}",
    )

    if stdout_match_mode == "exact":
        stdout_assertion = self.assertEqual
    elif stdout_match_mode == "contains":
        stdout_assertion = self.assertIn
    elif stdout_match_mode == "ignore":

        def stdout_assertion(*args, **kwargs):
            pass  # No action for 'ignore' mode

    else:
        raise ValueError(f"Invalid stdout_match_mode: {stdout_match_mode}")

    stdout_assertion(
        stdout_exp,
        result.stdout,
        f"stdout mismatch\n{error_context}"
        f"Expected : {repr(stdout_exp)} ({stdout_match_mode})\n"
        f"Actual:   {repr(result.stdout)}",
    )

    self.assertEqual(
        returncode_exp,
        result.returncode,
        f"return code mismatch\n{error_context}"
        f"Expected: {returncode_exp}\n"
        f"Actual:   {result.returncode}",
    )


def create_fresh_directory(dir_name: Path) -> Path:
    """
    Create a clean directory, removing it first if it already exists.

    Args:
        dir_name (Path): The name of the directory to create.

    Returns:
        Path: The path to the created directory.
    """

    if dir_name.exists():
        if dir_name.is_dir():
            shutil.rmtree(dir_name)
        else:
            raise FileNotFoundError(
                f"Error: Expected '{dir_name}' to be a directory, but found a file."
            )

    dir_name.mkdir(parents=True)
    return dir_name


def parse_args(argv=None) -> tuple[argparse.Namespace, List[str]]:
    """
    Parse command-line arguments for the CLI testing script.
    """
    parser = argparse.ArgumentParser(add_help=False)

    parser.add_argument(
        "--cli",
        type=Path,
        default=Path("../programs/airspace"),
        help="Path to the CLI to test",
        dest="cli_path",
    )
    parser.add_argument(
        "--test-root",
        type=Path,
        default=Path("."),
        help="Run the tests under this directory. Scratch directory is located in TEST_ROOT/scratch/",
        dest="test_root",
    )
    # parser.add_argument(
    #     "--preserve",
    #     action="store_true",
    #     help="Preserve the scratch directory TEST_ROOT/scratch/ for debugging purposes",
    #     dest="preserve_test_artifacts",
    # )
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="Show this help message and exit.",
        dest="help",
    )

    args, unit_argv = parser.parse_known_args(argv)

    if args.help:
        print(
            "Custom Modifications: This program extends the unittest with the following options:"
        )
        parser.print_help()
        print("\nNormal Python unittest help:")
        unit_argv = [sys.argv[0], "-h"]

    unit_argv = [sys.argv[0]] + unit_argv

    return args, unit_argv


# Add the custom CLI assert function to unittest.TestCase class
setattr(unittest.TestCase, "assertCli", assertCli)


def main(argv=None, **kwargs):
    args, unittest_argv = parse_args(argv)

    CliTest.cli_path = args.cli_path
    CliTest.test_root = args.test_root
    CliTest.cli_cwd = args.test_root

    unittest.main(argv=unittest_argv, **kwargs)
