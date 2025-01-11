# Copyright (c) 2018, 2019 Nordic Semiconductor ASA
# Copyright 2018, 2019 Foundries.io
#
# SPDX-License-Identifier: Apache-2.0

'''West project commands'''

import argparse
from functools import partial
import logging
import os
from os.path import abspath, relpath
from pathlib import PurePath, Path
import shutil
import shlex
import subprocess
import sys
import textwrap
import time
from time import perf_counter
from urllib.parse import urlparse

from west.configuration import Configuration
from west import util
from west.commands import WestCommand, CommandError, Verbosity
from west.manifest import ImportFlag, Manifest, \
    ManifestProject, _manifest_content_at, ManifestImportFailed
from west.manifest import is_group as is_project_group
from west.manifest import MANIFEST_REV_BRANCH as MANIFEST_REV
from west.manifest import Submodule
from west.manifest import QUAL_MANIFEST_REV_BRANCH as QUAL_MANIFEST_REV
from west.manifest import QUAL_REFS_WEST as QUAL_REFS

#
# Project-related or multi-repo commands, like "init", "update",
# "diff", etc.
#

class _ProjectCommand(WestCommand):
    # Helper class which contains common code needed by various commands
    # in this file.

    def _parser(self, parser_adder, **kwargs):
        # Create and return a "standard" parser.

        kwargs['help'] = self.help
        kwargs['description'] = self.description
        kwargs['formatter_class'] = argparse.RawDescriptionHelpFormatter
        return parser_adder.add_parser(self.name, **kwargs)

    def _cloned_projects(self, args, only_active=False):
        # Returns _projects(args.projects, only_cloned=True) if
        # args.projects is not empty (i.e., explicitly given projects
        # are required to be cloned). Otherwise, returns all cloned
        # projects.
        if args.projects:
            ret = self._projects(args.projects, only_cloned=True)
        else:
            ret = [p for p in self.manifest.projects if p.is_cloned()]

        if args.projects or not only_active:
            return ret

        return [p for p in ret if self.manifest.is_active(p)]

    def _projects(self, ids, only_cloned=False):
        try:
            return self.manifest.get_projects(ids, only_cloned=only_cloned)
        except ValueError as ve:
            if len(ve.args) != 2:
                raise          # not directly raised by get_projects()

            # Die with an error message on unknown or uncloned projects.
            unknown, uncloned = ve.args
            if unknown:
                self._die_unknown(unknown)
            elif only_cloned and uncloned:
                s = 's' if len(uncloned) > 1 else ''
                names = ' '.join(p.name for p in uncloned)
                self.die(f'uncloned project{s}: {names}.\n'
                         '  Hint: run "west update" and retry.')
            else:
                # Should never happen, but re-raise to fail fast and
                # preserve a stack trace, to encourage a bug report.
                raise

    def _handle_failed(self, args, failed):
        # Shared code for commands (like status, diff, update) that need
        # to do the same thing to multiple projects, but collect
        # and report errors if anything failed.

        if not failed:
            self.dbg(f'git {self.name} failed for zero project')
            return
        elif len(failed) < 20:
            s = 's:' if len(failed) > 1 else ''
            projects = ', '.join(f'{p.name}' for p in failed)
            self.err(f'{self.name} failed for project{s} {projects}')
        else:
            self.err(f'{self.name} failed for multiple projects; see above')
        raise CommandError(1)

    def _die_unknown(self, unknown):
        # Scream and die about unknown projects.

        s = 's' if len(unknown) > 1 else ''
        names = ' '.join(unknown)
        self.die(f'unknown project name{s}/path{s}: {names}\n'
                 '  Hint: use "west list" to list all projects.')

    def _has_nonempty_status(self, project):
        # Check if the project has any status output to print. We
        # manually use Popen in order to try to exit as quickly as
        # possible if 'git status' prints anything.

        popen = subprocess.Popen(['git', 'status', '--porcelain'],
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE,
                                 cwd=project.abspath)

        def has_output():
            # 'git status --porcelain' prints nothing if there
            # are no notable changes, so any output at all
            # means we should run 'git status' on the project.
            stdout, stderr = None, None
            try:
                stdout, stderr = popen.communicate(timeout=0.1)
            except subprocess.TimeoutExpired:
                pass
            return stdout or stderr

        while True:
            if has_output():
                popen.kill()
                return True
            if popen.poll() is not None:
                break

        return has_output()

class Init(_ProjectCommand):

    def __init__(self):
        super().__init__(
            'init',
            'create a west workspace',
            f'''\
Creates a west workspace.

With -l, creates a workspace around an existing local repository;
without -l, creates a workspace by cloning a manifest repository
by URL.

With -m, clones the repository at that URL and uses it as the
manifest repository. If --mr is not given, the remote's default
branch will be used, if it exists.

With neither, -m {MANIFEST_URL_DEFAULT} is assumed.

Warning: 'west init' renames and/or deletes temporary files inside the
workspace being created. This fails on some filesystems when some
development tool or any other program is trying to read/index these
temporary files at the same time. For instance, it is required to stop
Visual Studio Code before running 'west init` on the Windows NTFS
filesystem. Find other, similar "Access is denied" examples in west
issue #558.
This is not required with most Linux filesystems that have an inode
indirection layer and can wait to
finalize the deletion until there is no concurrent user left.
If you cannot identify or cannot stop the background scanner
that is interfering with renames on your system, try the --rename-delay hack
below.
''',
            requires_workspace=False)

    def do_add_parser(self, parser_adder):
        # We set a custom usage because there are two distinct ways
        # to call this command, and the default usage generated by
        # argparse doesn't make that very clear.

        parser = self._parser(
            parser_adder,
            usage='''

  %(prog)s [-m URL] [--mr REVISION] [--mf FILE] [-o=GIT_CLONE_OPTION] [directory]
  %(prog)s -l [--mf FILE] directory
''')

        # Remember to update the usage if you modify any arguments.

        parser.add_argument('-m', '--manifest-url',
                            help='''manifest repository URL to clone;
                            cannot be combined with -l''')
        parser.add_argument('-o', '--clone-opt', action='append', default=[],
                            help='''additional option to pass to 'git clone'
                            (e.g. '-o=--depth=1'); may be given more than once;
                            cannot be combined with -l''')
        parser.add_argument('--mr', '--manifest-rev', dest='manifest_rev',
                            help='''manifest repository branch or tag name
                            to check out first; cannot be combined with -l''')
        parser.add_argument('--mf', '--manifest-file', dest='manifest_file',
                            help='manifest file name to use')
        parser.add_argument('-l', '--local', action='store_true',
                            help='''use "directory" as an existing local
                            manifest repository instead of cloning one from
                            MANIFEST_URL; .west is created next to "directory"
                            in this case, and manifest.path points at
                            "directory"''')
        parser.add_argument('--rename-delay', type=int,
                            help='''Number of seconds to wait before renaming
                            some temporary directories. Some filesystems like NTFS
                            cannot rename files in use; see above. This is a HACK
                            that may or may not give enough time for some random
                            background scanner to complete. ''')

        parser.add_argument(
            'directory', nargs='?', default=None,
            help='''with -l, the path to the local manifest repository;
            without it, the directory to create the workspace in (defaulting
            to the current working directory in this case)''')

        return parser

    def do_run(self, args, _):
        if self.topdir:
            zb = os.environ.get('ZEPHYR_BASE')
            if zb:
                msg = textwrap.dedent(f'''
                Note:
                    In your environment, ZEPHYR_BASE is set to:
                    {zb}

                    This forces west to search for a workspace there.
                    Try unsetting ZEPHYR_BASE and re-running this command.''')
            else:
                west_dir = Path(self.topdir) / WEST_DIR
                msg = ("\n  Hint: if you do not want a workspace there, \n"
                       "  remove this directory and re-run this command:\n\n"
                       f"  {west_dir}")

            self.die_already(self.topdir, msg)

        if args.local and (args.manifest_url or args.manifest_rev or args.clone_opt):
            self.die('-l cannot be combined with -m, -o or --mr')

        self.die_if_no_git()

        if args.local:
            topdir = self.local(args)
        else:
            topdir = self.bootstrap(args)

        self.banner(f'Initialized. Now run "west update" inside {topdir}.')

    def die_already(self, where, also=None):
        self.die(f'already initialized in {where}, aborting.{also or ""}')

    def local(self, args) -> Path:
        if args.manifest_rev is not None:
            self.die('--mr cannot be used with -l')

        # We need to resolve this to handle the case that args.directory
        # is '.'. In that case, Path('.').parent is just Path('.') instead of
        # Path('..').
        #
        # https://docs.python.org/3/library/pathlib.html#pathlib.PurePath.parent
        manifest_dir = Path(args.directory or os.getcwd()).resolve()
        manifest_filename = args.manifest_file or 'west.yml'
        manifest_file = manifest_dir / manifest_filename
        topdir = manifest_dir.parent
        rel_manifest = manifest_dir.name
        west_dir = topdir / WEST_DIR

        if not manifest_file.is_file():
            self.die(f'can\'t init: no {manifest_filename} found in '
                     f'{manifest_dir}')

        self.banner('Initializing from existing manifest repository',
                    rel_manifest)
        self.small_banner(f'Creating {west_dir} and local configuration file')
        self.create(west_dir)
        os.chdir(topdir)
        self.config = Configuration(topdir=topdir)
        self.config.set('manifest.path', os.fspath(rel_manifest))
        self.config.set('manifest.file', manifest_filename)

        return topdir

    def bootstrap(self, args) -> Path:
        topdir = Path(abspath(args.directory or os.getcwd()))
        self.banner('Initializing in', topdir)

        manifest_url = args.manifest_url or MANIFEST_URL_DEFAULT
        if args.manifest_rev:
            # This works with tags, too.
            branch_opt = ['--branch', args.manifest_rev]
        else:
            branch_opt = []
        west_dir = topdir / WEST_DIR

        try:
            already = util.west_topdir(topdir, fall_back=False)
            self.die_already(already)
        except util.WestNotFound:
            pass

        if not topdir.is_dir():
            self.create(topdir, exist_ok=False)

        tempdir: Path = west_dir / 'manifest-tmp'
        if tempdir.is_dir():
            self.dbg('removing existing temporary manifest directory', tempdir)
            shutil.rmtree(tempdir)

        # Test that we can rename and delete directories. For the vast
        # majority of users this is a no-op but some filesystem
        # permissions can be weird; see October 2024 example in west
        # issue #558.  Git cloning can take a long time, so check this
        # first.  Failing ourselves is not just faster, it's also much
        # clearer than when git is involved in the mix.

        tempdir.mkdir(parents=True)
        (tempdir / 'not empty').mkdir()
        # Ignore the --rename-delay hack here not to double the wait;
        # we only have a couple directories and no file at this point!
        tempdir2 = tempdir.parent / 'renamed tempdir'
        os.rename(tempdir, tempdir2)
        # No need to delete west_dir parent
        shutil.rmtree(tempdir2)

        # Clone the manifest repository into a temporary directory.
        try:
            self.small_banner(
                f'Cloning manifest repository from {manifest_url}' +
                (f', rev. {args.manifest_rev}' if args.manifest_rev else ''))

            self.check_call(['git', 'clone'] + branch_opt + args.clone_opt +
                            [manifest_url, os.fspath(tempdir)])
        except subprocess.CalledProcessError:
            shutil.rmtree(tempdir, ignore_errors=True)
            raise

        # Verify the manifest file exists.
        temp_manifest_filename = args.manifest_file or 'west.yml'
        temp_manifest = tempdir / temp_manifest_filename
        if not temp_manifest.is_file():
            self.die(f'can\'t init: no {temp_manifest_filename} found in '
                     f'{tempdir}\n'
                     f'  Hint: check --manifest-url={manifest_url}' +
                     (f' and --manifest-rev={args.manifest_rev}'
                      if args.manifest_rev else '') +
                     f'  You may need to remove {west_dir} before retrying.')

        # Parse the manifest to get "self: path:", if it declares one.
        # Otherwise, use the URL. Ignore imports -- all we really
        # want to know is if there's a "self: path:" or not.
        manifest = Manifest.from_data(temp_manifest.read_text(encoding=Manifest.encoding),
                                      import_flags=ImportFlag.IGNORE)
        if manifest.yaml_path:
            manifest_path = manifest.yaml_path
        else:
            # We use PurePath() here in case manifest_url is a
            # windows-style path. That does the right thing in that
            # case, without affecting POSIX platforms, where PurePath
            # is PurePosixPath.
            manifest_path = PurePath(urlparse(manifest_url).path).name

        manifest_abspath = topdir / manifest_path

        # Some filesystems like NTFS can't rename files in use.
        # See west issue #558. Will ReFS address this?
        ren_delay = args.rename_delay
        if ren_delay is not None:
            self.inf(f"HACK: waiting {ren_delay} seconds before renaming {tempdir}")
            time.sleep(ren_delay)

        self.dbg('moving', tempdir, 'to', manifest_abspath,
                 level=Verbosity.DBG_EXTREME)

        # As shutil.move() is used to relocate tempdir, if manifest_abspath
        # is an existing directory, tmpdir will be moved _inside_ it, instead
        # of _to_ that path - this must be avoided. If manifest_abspath exists
        # but is not a directory, then semantics depend on os.rename(), so
        # avoid that too...
        if manifest_abspath.exists():
            self.die(f'target directory already exists ({manifest_abspath})')

        manifest_abspath.parent.mkdir(parents=True, exist_ok=True)
        try:
            shutil.move(os.fspath(tempdir), os.fspath(manifest_abspath))
        except shutil.Error as e:
            self.die(e)
        self.small_banner('setting manifest.path to', manifest_path)
        self.config = Configuration(topdir=topdir)
        self.config.set('manifest.path', manifest_path)
        self.config.set('manifest.file', temp_manifest_filename)

        return topdir

    def create(self, directory: Path, exist_ok: bool = True) -> None:
        try:
            directory.mkdir(parents=True, exist_ok=exist_ok)
        except PermissionError:
            self.die(f'Cannot initialize in {directory}: permission denied')
        except FileExistsError:
            self.die(f'Cannot initialize in {directory}: it already exists')
        except Exception as e:
            self.die(f"Can't create {directory}: {e}")

class List(_ProjectCommand):
    def __init__(self):
        super().__init__(
            'list',
            'print information about projects',
            textwrap.dedent('''\
            Print information about projects in the west manifest,
            using format strings.'''))

    def do_add_parser(self, parser_adder):
        default_fmt = '{name:12} {path:28} {revision:40} {url}'
        parser = self._parser(
            parser_adder,
            epilog=f'''\
{ACTIVE_PROJECTS_HELP}

FORMAT STRINGS
--------------

Projects are listed using a Python 3 format string. Arguments
to the format string are accessed by name.

The default format string is:

"{default_fmt}"

The following arguments are available:

- name: project name in the manifest
- description: project description in the manifest
- url: full remote URL as specified by the manifest
- path: the relative path to the project from the top level,
  as specified in the manifest where applicable
- abspath: absolute and normalized path to the project
- posixpath: like abspath, but in posix style, that is, with '/'
  as the separator character instead of '\\'
- revision: project's revision as it appears in the manifest
- sha: project's revision as a SHA. Note that use of this requires
  that the project has been cloned.
- cloned: "cloned" if the project has been cloned, "not-cloned"
  otherwise
- clone_depth: project clone depth if specified, "None" otherwise
- groups: project groups, as a comma-separated list
''')
        parser.add_argument('-a', '--all', action='store_true',
                            help='include inactive projects'),
        parser.add_argument('--manifest-path-from-yaml', action='store_true',
                            help='''print the manifest repository's path
                            according to the manifest file YAML, which may
                            disagree with the manifest.path configuration
                            option'''),
        parser.add_argument('-f', '--format', default=default_fmt,
                            help='''format string to use to list each
                            project; see FORMAT STRINGS below.''')

        parser.add_argument('projects', metavar='PROJECT', nargs='*',
                            help='''projects (by name or path) to operate on;
                            see ACTIVE PROJECTS below''')

        return parser

    def do_run(self, args, user_args):
        def sha_thunk(project):
            self.die_if_no_git()

            if not project.is_cloned():
                self.die(
                    f'cannot get sha for uncloned project {project.name}; '
                    f'run "west update {project.name}" and retry')
            elif isinstance(project, ManifestProject):
                return f'{"N/A":40}'
            else:
                return project.sha(MANIFEST_REV)

        def cloned_thunk(project):
            self.die_if_no_git()

            return "cloned" if project.is_cloned() else "not-cloned"

        def delay(func, project):
            return DelayFormat(partial(func, project))

        for project in self._projects(args.projects):
            # Skip inactive projects unless the user said
            # --all or named some projects explicitly.
            if not (args.all or args.projects or
                    self.manifest.is_active(project)):
                self.dbg(f'{project.name}: skipping inactive project')
                continue

            # Spelling out the format keys explicitly here gives us
            # future-proofing if the internal Project representation
            # ever changes.
            #
            # Using DelayFormat delays computing derived values, such
            # as SHAs, unless they are specifically requested, and then
            # ensures they are only computed once.
            try:
                if isinstance(project, ManifestProject):
                    # Special-case the manifest repository while it's
                    # still showing up in the 'projects' list. Yet
                    # more evidence we should tackle #327.
                    if args.manifest_path_from_yaml:
                        path = self.manifest.yaml_path
                        apath = (abspath(os.path.join(self.topdir, path))
                                 if path else None)
                        ppath = Path(apath).as_posix() if apath else None
                    else:
                        path = self.manifest.repo_path
                        apath = self.manifest.repo_abspath
                        ppath = self.manifest.repo_posixpath
                else:
                    path = project.path
                    apath = project.abspath
                    ppath = project.posixpath

                result = args.format.format(
                    name=project.name,
                    description=project.description or "None",
                    url=project.url or 'N/A',
                    path=path,
                    abspath=apath,
                    posixpath=ppath,
                    revision=project.revision or 'N/A',
                    clone_depth=project.clone_depth or "None",
                    cloned=delay(cloned_thunk, project),
                    sha=delay(sha_thunk, project),
                    groups=','.join(project.groups))
            except KeyError as e:
                # The raised KeyError seems to just put the first
                # invalid argument in the args tuple, regardless of
                # how many unrecognizable keys there were.
                self.die(f'unknown key "{e.args[0]}" in format string '
                         f'{shlex.quote(args.format)}')
            except IndexError:
                self.parser.print_usage()
                self.die(f'invalid format string {shlex.quote(args.format)}')
            except subprocess.CalledProcessError:
                self.die(f'subprocess failed while listing {project.name}')

            self.inf(result, colorize=False)

class ManifestCommand(_ProjectCommand):
    # The slightly weird naming is to avoid a conflict with
    # west.manifest.Manifest.

    def __init__(self):
        super(ManifestCommand, self).__init__(
            'manifest',
            'manage the west manifest',
            textwrap.dedent('''\
            Manages the west manifest.

            The following actions are available. You must give exactly one.

            - --resolve: print the current manifest with all imports applied,
              as an equivalent single manifest file. Any imported manifests
              must be cloned locally (with "west update").

            - --freeze: like --resolve, but with all project revisions
              converted to their current SHAs, based on the latest manifest-rev
              branches. All projects must be cloned (with "west update").

            - --validate: print an error and exit the process unsuccessfully
              if the current manifest cannot be successfully parsed.
              If the manifest can be parsed, print nothing and exit
              successfully.

            - --path: print the path to the top level manifest file.
              If this file uses imports, it will not contain all the
              manifest data.

            If the manifest file does not use imports, and all project
            revisions are SHAs, the --freeze and --resolve output will
            be identical after a "west update".
            '''),
            accepts_unknown_args=False)

    def do_add_parser(self, parser_adder):
        parser = self._parser(parser_adder)

        group = parser.add_mutually_exclusive_group(required=True)
        group.add_argument('--resolve', action='store_true',
                           help='print the manifest with all imports resolved')
        group.add_argument('--freeze', action='store_true',
                           help='''print the resolved manifest with SHAs for
                           all project revisions''')
        group.add_argument('--validate', action='store_true',
                           help='''validate the current manifest,
                           exiting with an error if there are issues''')
        group.add_argument('--path', action='store_true',
                           help="print the top level manifest file's path")

        group = parser.add_argument_group('options for --resolve and --freeze')
        group.add_argument('-o', '--out',
                           help='output file, default is standard output')

        return parser

    def do_run(self, args, user_args):
        manifest = self.manifest
        dump_kwargs = {'default_flow_style': False,
                       'sort_keys': False}

        if args.validate:
            pass              # nothing more to do
        elif args.resolve:
            self._die_if_manifest_project_filter('resolve')
            self._dump(args, manifest.as_yaml(**dump_kwargs))
        elif args.freeze:
            self._die_if_manifest_project_filter('freeze')
            self._dump(args, manifest.as_frozen_yaml(**dump_kwargs))
        elif args.path:
            self.inf(manifest.path)
        else:
            # Can't happen.
            raise RuntimeError(f'internal error: unhandled args {args}')

    def _die_if_manifest_project_filter(self, action):
        if self.config.get('manifest.project-filter') is not None:
            self.die(f'"west manifest --{action}" is not (yet) supported '
                     'when the manifest.project-filter option is set. '
                     'Please clear the project-filter configuration '
                     'option and re-run this command, or contact the '
                     'west developers if you have a use case for resolving '
                     'the manifest while projects are made inactive by the '
                     'project filter.')

    def _dump(self, args, to_dump):
        if args.out:
            with open(args.out, 'w') as f:
                f.write(to_dump)
        else:
            sys.stdout.write(to_dump)

class Compare(_ProjectCommand):
    def __init__(self):
        super().__init__(
            'compare',
            "compare project status against the manifest",
            textwrap.dedent('''\
            Compare each project's working tree state against the results
            of the most recent successful "west update" command.

            This command prints output for a project if (and only if)
            at least one of the following is true:

            1. its checked out commit (HEAD) is different than the commit
               checked out by the most recent successful "west update"
               (which the manifest-rev branch will point to)
            2. its working tree is not clean (i.e. there are local uncommitted
               changes)
            3. it has a local checked out branch (unless the configuration
               option compare.ignore-branches is true or --ignore-branches
               is given on the command line, either of which disable this)

            The command also prints output for the manifest repository if it
            has nonempty status.

            The output is meant to be human-readable, and may change. It is
            not a stable interface to write scripts against. This command
            requires git 2.22 or later.''')
        )

    def do_add_parser(self, parser_adder):
        parser = self._parser(parser_adder,
                              epilog=ACTIVE_CLONED_PROJECTS_HELP)
        parser.add_argument('projects', metavar='PROJECT', nargs='*',
                            help='''projects (by name or path) to operate on;
                            defaults to active cloned projects''')
        parser.add_argument('-a', '--all', action='store_true',
                            help='include output for inactive projects')
        parser.add_argument('--exit-code', action='store_true',
                            help='''exit with status code 1 if status output
                            was printed for any project''')
        parser.add_argument('--ignore-branches',
                            default=None, action='store_true',
                            help='''skip output for projects with checked out
                            branches and clean working trees if the branch is
                            at the same commit as the last "west update"''')
        parser.add_argument('--no-ignore-branches', dest='ignore_branches',
                            action='store_false',
                            help='''overrides a previous --ignore-branches
                            or any compare.ignore-branches configuration
                            option''')
        return parser

    def do_run(self, args, ignored):
        self.die_if_no_git()
        if self.git_version_info < (2, 22):
            # This is for git branch --show-current.
            self.die('git version 2.22 or later is required')

        if args.ignore_branches is not None:
            self.ignore_branches = args.ignore_branches
        else:
            self.ignore_branches = \
                self.config.getboolean('compare.ignore-branches', False)

        failed = []
        printed_output = False
        for project in self._cloned_projects(args, only_active=not args.all):
            if isinstance(project, ManifestProject):
                # West doesn't track the relationship between the manifest
                # repository and any remote, but users are still interested
                # in printing output for comparisons that makes sense.
                if self._has_nonempty_status(project):
                    try:
                        self.compare(project)
                        printed_output = True
                    except subprocess.CalledProcessError:
                        failed.append(project)
                continue

            # 'git status' output for all projects is noisy when there
            # are lots of projects.
            #
            # We avoid this problem in 2 steps:
            #
            #   1. Check if we need to print any output for the
            #      project.
            #
            #   2. If so, run 'git status' on the project. Otherwise,
            #      skip output for the project entirely.
            #
            # In verbose mode, we always print output.

            def has_checked_out_branch(project):
                if self.ignore_branches:
                    return False

                return bool(project.git(['branch', '--show-current'],
                                        capture_stdout=True,
                                        capture_stderr=True).stdout.strip())

            try:
                if not (self.verbosity >= Verbosity.DBG or
                        has_checked_out_branch(project) or
                        (project.sha(QUAL_MANIFEST_REV) !=
                         project.sha('HEAD')) or
                        self._has_nonempty_status(project)):
                    continue

                self.compare(project)
                printed_output = True
            except subprocess.CalledProcessError:
                failed.append(project)
        self._handle_failed(args, failed)

        if args.exit_code and printed_output:
            raise CommandError(1)

    def compare(self, project):
        self.banner(f'{project.name_and_path}:')
        self.print_rev_info(project)
        self.print_status(project)

    def print_rev_info(self, project):
        # For non-manifest repositories, print HEAD's and
        # manifest-rev's SHAs and commit titles.
        #
        # We force git not to print in color so west's colored
        # banner() separators stand out more in the output.
        if isinstance(project, ManifestProject):
            return

        def rev_info(rev):
            title = project.git(
                ['log', '-1', '--color=never', '--pretty=%h%d %s',
                 '--decorate-refs-exclude=refs/heads/manifest-rev',
                 rev],
                capture_stdout=True,
                capture_stderr=True).stdout.decode().rstrip()
            # "HEAD" is special; '--decorate-refs-exclude=HEAD' doesn't work.
            # Fortunately it's always first.
            return (
                title.replace('(HEAD) ', '').replace('(HEAD, ', '(')
                .replace('(HEAD -> ', '(')
            )

        head_info = rev_info('HEAD')
        # If manifest-rev is missing, we already failed earlier.
        manifest_rev_info = rev_info('manifest-rev')
        self.small_banner(f'manifest-rev: {manifest_rev_info}')
        self.inf(f'            HEAD: {head_info}')

    def print_status(self, project):
        # `git status` shows `manifest-rev` "sometimes", see #643.
        if self.color_ui:
            color = '-c status.color=always'
        else:
            color = ''
        cp = project.git(f'{color} status', capture_stdout=True,
                         capture_stderr=True)
        self.small_banner('status:')
        self.inf(textwrap.indent(cp.stdout.decode().rstrip(), ' ' * 4))

class Diff(_ProjectCommand):
    def __init__(self):
        super().__init__(
            'diff',
            '"git diff" for one or more projects',
            '''Runs "git diff" on each of the specified projects.
            Unknown arguments are passed to "git diff".''',
            accepts_unknown_args=True,
        )

    def do_add_parser(self, parser_adder):
        parser = self._parser(parser_adder,
                              epilog=ACTIVE_CLONED_PROJECTS_HELP)
        parser.add_argument('projects', metavar='PROJECT', nargs='*',
                            help='''projects (by name or path) to operate on;
                            defaults to active cloned projects''')
        parser.add_argument('-a', '--all', action='store_true',
                            help='include output for inactive projects')
        parser.add_argument('-m', '--manifest', action='store_true',
                            help='show changes relative to "manifest-rev"')
        return parser

    def do_run(self, args, user_args):
        self.die_if_no_git()

        failed = []
        no_diff = 0
        # We may need to force git to use colors if the user wants them,
        # which it won't do ordinarily since stdout is not a terminal.
        color = ['--color=always'] if self.color_ui else []
        for project in self._cloned_projects(args, only_active=not args.all):
            diff_commit = (
                ['manifest-rev'] # see #719 and #747
                # Special-case the manifest repository while it's
                # still showing up in the 'projects' list. Yet
                # more evidence we should tackle #327.
                if args.manifest and not isinstance(project, ManifestProject)
                else []
            )
            # Use paths that are relative to the base directory to make it
            # easier to see where the changes are
            cp = project.git(['diff', f'--src-prefix={project.path}/',
                              f'--dst-prefix={project.path}/',
                              '--exit-code'] + color + diff_commit,
                             extra_args=user_args,
                             capture_stdout=True, capture_stderr=True,
                             check=False)

            # We cannot trust --exit-code alone, for instance merge
            # conflicts return 0 with (at least) git version 2.46.0. See
            # west issue #731
            some_diff = cp.returncode == 1 or (cp.returncode == 0 and len(cp.stdout) > 0)
            if not some_diff:
                no_diff += 1
            if some_diff or self.verbosity >= Verbosity.DBG:
                self.banner(f'diff for {project.name_and_path}:')
                self.inf(cp.stdout.decode('utf-8'))
                self.inf(cp.stderr.decode('utf-8'))
            if cp.returncode > 1:
                failed.append(project)

        if failed:
            self._handle_failed(args, failed)
        elif self.verbosity <= Verbosity.INF:
            self.inf(f"Empty diff in {no_diff} projects.")

class Status(_ProjectCommand):
    def __init__(self):
        super().__init__(
            'status',
            '"git status" for one or more projects',
            '''Runs "git status" for each of the specified projects.
            Unknown arguments are passed to "git status".''',
            accepts_unknown_args=True,
        )

    def do_add_parser(self, parser_adder):
        parser = self._parser(parser_adder,
                              epilog=ACTIVE_CLONED_PROJECTS_HELP)
        parser.add_argument('projects', metavar='PROJECT', nargs='*',
                            help='''projects (by name or path) to operate on;
                            defaults to active cloned projects''')
        parser.add_argument('-a', '--all', action='store_true',
                            help='include output for inactive projects')
        return parser

    def do_run(self, args, user_args):
        self.die_if_no_git()

        failed = []
        for project in self._cloned_projects(args, only_active=not args.all):
            # 'git status' output for all projects is noisy when there
            # are lots of projects.
            #
            # We avoid this problem in 2 steps:
            #
            #   1. Check if we need to print any output for the
            #      project.
            #
            #   2. If so, run 'git status' on the project. Otherwise,
            #      skip output for the project entirely.
            #
            # In verbose mode, we always print output.

            try:
                if not (self.verbosity >= Verbosity.DBG or
                        self._has_nonempty_status(project)):
                    continue

                self.banner(f'status of {project.name_and_path}:')
                project.git('status', extra_args=user_args)
            except subprocess.CalledProcessError:
                failed.append(project)
        self._handle_failed(args, failed)

class Update(_ProjectCommand):

    def __init__(self):
        super().__init__(
            'update',
            'update projects described in west manifest',
            textwrap.dedent('''\
            Updates active projects defined in the manifest file as follows:

            1. Clone the project if necessary
            2. If necessary, fetch the project's revision from its remote
               (see "fetching behavior" below)
            3. Reset the manifest-rev branch to the current manifest revision
            4. Check out the new manifest-rev commit as a detached
               HEAD (the default), or keep/rebase existing checked out branches
               (see "checked out branch behavior")

            You must have already created a west workspace with "west init".

            This command does not alter the manifest repository's contents.''')
        )

    def do_add_parser(self, parser_adder):
        parser = self._parser(parser_adder)

        parser.add_argument('--stats', action='store_true',
                            help='''print performance statistics for
                            update operations''')

        group = parser.add_argument_group(
            title='local project clone caches',
            description=textwrap.dedent('''\
            Projects are usually initialized by fetching from their URLs, but
            they can also be cloned from caches on the local file system.'''))
        group.add_argument('--name-cache',
                           help='''cached repositories are in subdirectories
                           matching the names of projects to update''')
        group.add_argument('--path-cache',
                           help='''cached repositories are in the same relative
                           paths as the workspace being updated''')

        group = parser.add_argument_group(
            title='fetching behavior',
            description='By default, west update tries to avoid fetching.')
        group.add_argument('-f', '--fetch', dest='fetch_strategy',
                           choices=['always', 'smart'],
                           help='''how to fetch projects when updating:
                           "always" fetches every project before update,
                           while "smart" (default) skips fetching projects
                           whose revisions are SHAs or tags available
                           locally''')
        group.add_argument('-o', '--fetch-opt', action='append', default=[],
                           help='''additional option to pass to 'git fetch'
                           if fetching is necessary (e.g. 'o=--depth=1');
                           may be given more than once''')
        group.add_argument('-n', '--narrow', action='store_true',
                           help='''fetch just the project revision if fetching
                           is necessary; do not pass --tags to git fetch
                           (may not work for SHA revisions depending on the Git
                           host)''')

        group = parser.add_argument_group(
            title='checked out branch behavior',
            description=textwrap.dedent('''\
            By default, locally checked out branches are left behind
            when manifest-rev commits are checked out.'''))
        group.add_argument('-k', '--keep-descendants', action='store_true',
                           help='''if a checked out branch is a descendant
                           of the new manifest-rev, leave it checked out
                           instead (takes priority over --rebase)''')
        group.add_argument('-r', '--rebase', action='store_true',
                           help='''rebase any checked out branch onto the new
                           manifest-rev instead (leaving behind partial
                           rebases on error)''')

        group = parser.add_argument_group(
            title='advanced options')
        group.add_argument('--group-filter', '--gf', action='append',
                           default=[], metavar='FILTER', dest='group_filter',
                           help='''proceed as if FILTER was appended to
                           manifest.group-filter; may be given multiple
                           times''')
        group.add_argument('--submodule-init-config',
                           action='append', default=[],
                           help='''git configuration option to set when running
                           'git submodule init' in '<option>=<value>' format;
                           may be given more than once''')

        group = parser.add_argument_group('deprecated options')
        group.add_argument('-x', '--exclude-west', action='store_true',
                           help='ignored for backwards compatibility')

        parser.add_argument('projects', metavar='PROJECT', nargs='*',
                            help='''projects (by name or path) to operate on;
                            defaults to all active projects''')

        return parser

    def do_run(self, args, _):
        self.die_if_no_git()
        self.init_state(args)

        # We can't blindly call self._projects() here: manifests with
        # imports are limited to plain 'west update', and cannot use
        # 'west update PROJECT [...]'.
        if not self.args.projects:
            self.update_all()
        else:
            self.update_some()

    def init_state(self, args):
        # Helper for initializing instance state in response to
        # command line args and configuration files.

        self.args = args
        if args.exclude_west:
            self.wrn('ignoring --exclude-west')

        config = self.config
        self.narrow = args.narrow or config.getboolean('update.narrow')
        self.path_cache = args.path_cache or config.get('update.path-cache')
        self.name_cache = args.name_cache or config.get('update.name-cache')
        self.sync_submodules = config.getboolean('update.sync-submodules',
                                                 default=True)

        self.group_filter: List[str] = []

        def handle(group_filter_item):
            item = group_filter_item.strip()
            if not item.startswith(('-', '+')):
                self.die(f'invalid --group-filter item {item}: '
                         'must start with - or +')
            if not is_project_group(item[1:]):
                self.die(f'invalid --group-filter item {item}: '
                         f'"{item[1:]}" is not a valid group name')
            self.group_filter.append(item)

        for item in args.group_filter:
            if ',' in item:
                for split_item in item.split(','):
                    handle(split_item)
            else:
                handle(item)

        self.fs = self.fetch_strategy()

    def update_all(self):
        # Plain 'west update' is the 'easy' case: since the user just
        # wants us to update everything, we don't have to keep track
        # of projects appearing or disappearing as a result of fetching
        # new revisions from projects with imports.
        #
        # So we just re-parse the manifest, but force west.manifest to
        # call our importer whenever it encounters an import statement
        # in a project, allowing us to control the recursion so it
        # always uses the latest manifest data.
        self.updated = set()

        self.manifest = Manifest.from_file(
            importer=self.update_importer,
            import_flags=ImportFlag.FORCE_PROJECTS)

        failed = []
        for project in self.manifest.projects:
            if (isinstance(project, ManifestProject) or
                    project.name in self.updated):
                continue
            try:
                if not self.project_is_active(project):
                    self.dbg(f'{project.name}: skipping inactive project')
                    continue
                self.update(project)
                self.updated.add(project.name)
            except subprocess.CalledProcessError:
                failed.append(project)
        self._handle_failed(self.args, failed)

    def update_importer(self, project, path):
        if isinstance(project, ManifestProject):
            if not project.is_cloned():
                self.die("manifest repository {project.abspath} was deleted")
        else:
            # There's no need to call self.project_is_active(),
            # because the Manifest API guarantees that 'groups' cannot
            # be combined with 'import' within a single project.
            #
            # That's good, because the semantics would be kind of hard
            # to specify in this case.
            assert not project.groups

            self.update(project)
        self.updated.add(project.name)

        try:
            return _manifest_content_at(project, path, Manifest.encoding)
        except FileNotFoundError:
            # FIXME we need each project to have back-pointers
            # to the manifest file where it was defined, so we can
            # tell the user better context than just "run -vvv", which
            # is a total fire hose.
            name = project.name
            sha = project.sha(QUAL_MANIFEST_REV)
            if self.verbosity < Verbosity.DBG_EXTREME:
                suggest_vvv = ('\n'
                               '        Use "west -vvv update" to debug.')
            else:
                suggest_vvv = ''
            self.die(f"can't import from project {name}\n"
                     f'  Expected to import from {path} at revision {sha}\n'
                     f'  Hint: possible manifest file fixes for {name}:\n'
                     f'          - set "revision:" to a git ref with this '
                     f'file at URL {project.url}\n'
                     '          - remove the "import:"' + suggest_vvv)

    def update_some(self):
        # The 'west update PROJECT [...]' style invocation is only
        # implemented for projects defined within the manifest
        # repository.
        #
        # It's unclear how to do this properly in the case of
        # a project A whose definition is imported from
        # another project B, especially when B.revision is not
        # a fixed SHA. Do we forcibly need to update B first?
        # Should we skip it? Should it be configurable? Etc.
        #
        # For now, just refuse to do so. We can try to relax
        # this restriction if it proves cumbersome.

        if not self.has_manifest or self.manifest.has_imports:
            projects = self.toplevel_projects()
            assert self.has_manifest  # toplevel_projects() must ensure this.
        else:
            projects = self._projects(self.args.projects)

        failed = []
        for project in projects:
            if isinstance(project, ManifestProject):
                continue
            try:
                self.update(project)
            except subprocess.CalledProcessError:
                failed.append(project)
        self._handle_failed(self.args, failed)

    def toplevel_projects(self):
        # Return a list of projects from self.args.projects, or scream
        # and die if any projects are either unknown or not defined in
        # the manifest repository.
        #
        # As a side effect, ensures self.manifest is set.

        ids = self.args.projects
        assert ids

        self.manifest = Manifest.from_file(
            import_flags=ImportFlag.IGNORE_PROJECTS)
        mr_projects, mr_unknown = projects_unknown(self.manifest, ids)
        if not mr_unknown:
            return mr_projects

        try:
            self.manifest = Manifest.from_file()
        except ManifestImportFailed:
            self.die('one or more projects are unknown or defined via '
                     'imports; please run plain "west update".')

        _, unknown = projects_unknown(self.manifest, ids)
        if unknown:
            self._die_unknown(unknown)
        else:
            # All of the ids are known projects, but some of them
            # are not defined in the manifest repository.
            mr_unknown_set = set(mr_unknown)
            from_projects = [p for p in ids if p in mr_unknown_set]
            self.die('refusing to update project: ' +
                     " ".join(from_projects) + '\n' +
                     '  It or they were resolved via project imports.\n'
                     '  Only plain "west update" can currently update them.')

    def fetch_strategy(self):
        cfg = self.config.get('update.fetch')
        if cfg is not None and cfg not in ('always', 'smart'):
            self.wrn(f'ignoring invalid config update.fetch={cfg}; '
                     'choices: always, smart')
            cfg = None
        if self.args.fetch_strategy:
            return self.args.fetch_strategy
        elif cfg:
            return cfg
        else:
            return 'smart'

    def update_submodules(self, project):
        # Updates given project submodules by using
        # 'git submodule update --init --checkout --recursive' command
        # from the project.path location.
        if not project.submodules:
            return

        submodules = project.submodules
        submodules_update_strategy = ('--rebase' if self.args.rebase
                                      else '--checkout')
        config_opts = []
        for config_opt in self.args.submodule_init_config:
            config_opts.extend(['-c', config_opt])

        cache_dir = self.project_cache(project)
        # For the boolean type, update all the submodules.
        if isinstance(submodules, bool):
            if cache_dir is None:
                if self.sync_submodules:
                    project.git(['submodule', 'sync', '--recursive'])
                project.git(config_opts +
                            ['submodule', 'update', '--init',
                                submodules_update_strategy, '--recursive'])
                return
            else:
                # Cache used so convert to a list so that --reference can be used.
                res = project.git(['submodule', 'status'], capture_stdout=True)
                if not res.stdout or res.returncode:
                    self.die(
                        f"Submodule status failed for project: {project.name}.")
                mod_list = [s.strip() for s in res.stdout.decode('utf-8').split('\n') if s]
                submodules = [Submodule(line.split(' ')[1]) for line in mod_list]

        # For the list type, update given list of submodules.
        for submodule in submodules:
            if self.sync_submodules:
                project.git(['submodule', 'sync', '--recursive',
                             '--', submodule.path])
            ref = []
            if (cache_dir):
                submodule_ref = Path(cache_dir, submodule.path)
                if any(os.scandir(submodule_ref)):
                    ref = ['--reference', os.fspath(submodule_ref)]
                    self.small_banner(f'using reference from: {submodule_ref}')
                    self.dbg(
                        f'found {submodule.path} in --path-cache {submodule_ref}',
                        level=Verbosity.DBG_MORE)
            project.git(config_opts +
                        ['submodule', 'update',
                            '--init', submodules_update_strategy,
                            '--recursive'] + ref + [submodule.path])

    def update(self, project):
        if self.args.stats:
            stats = dict()
            update_start = perf_counter()
        else:
            stats = None
        take_stats = stats is not None

        self.banner(f'updating {project.name_and_path}:')

        # Make sure we've got a project to work with.
        self.ensure_cloned(project, stats, take_stats)

        # Point refs/heads/manifest-rev at project.revision,
        # fetching it from the remote if necessary.
        self.set_new_manifest_rev(project, stats, take_stats)

        # Clean up refs/west/*. At some point, we should only do this
        # if we've fetched, but we're leaving it here to clean up
        # garbage in people's repositories introduced by previous
        # versions of west that left refs in place here.
        self.clean_refs_west(project, stats, take_stats)

        # Make sure HEAD is pointing at *something*.
        self.ensure_head_ok(project, stats, take_stats)

        # Convert manifest-rev to a SHA.
        sha = self.manifest_rev_sha(project, stats, take_stats)

        # Based on the new manifest-rev SHA, HEAD, and the --rebase
        # and --keep-descendants options, decide what we need to do
        # now.
        current_branch, is_ancestor, try_rebase = self.decide_update_strategy(
            project, sha, stats, take_stats)

        # Finish the update. This may be a nop if we're keeping
        # descendants.
        if self.args.keep_descendants and is_ancestor:
            # A descendant is currently checked out and keep_descendants was
            # given, so there's nothing more to do.
            self.inf(f'west update: left descendant branch '
                     f'"{current_branch}" checked out; current status:')
            if take_stats:
                start = perf_counter()
            project.git('status')
            if take_stats:
                stats['get current status'] = perf_counter - start
        elif try_rebase:
            # Attempt a rebase.
            self.inf(f'west update: rebasing to {MANIFEST_REV} {sha}')
            if take_stats:
                start = perf_counter()
            project.git('rebase ' + QUAL_MANIFEST_REV)
            if take_stats:
                stats['rebase onto new manifest-rev'] = perf_counter() - start
        else:
            # We can't keep a descendant or rebase, so just check
            # out the new detached HEAD, then print some helpful context.
            if take_stats:
                start = perf_counter()
            project.git(['checkout', '--detach', sha])
            if take_stats:
                stats['checkout new manifest-rev'] = perf_counter() - start
            self.post_checkout_help(project, current_branch,
                                    sha, is_ancestor)

        # Update project submodules, if it has any.
        if take_stats:
            start = perf_counter()
        self.update_submodules(project)
        if take_stats:
            stats['update submodules'] = perf_counter() - start

        # Print performance statistics.
        if take_stats:
            update_total = perf_counter() - update_start
            slop = update_total - sum(stats.values())
            stats['other work'] = slop
            stats['TOTAL'] = update_total
            self.inf('performance statistics:')
            for stat, value in stats.items():
                self.inf(f'  {stat}: {value} sec')

    def post_checkout_help(self, project, branch, sha, is_ancestor):
        # Print helpful information to the user about a project that
        # might have just left a branch behind.

        if branch == 'HEAD':
            # If there was no branch checked out, there are no
            # additional diagnostics that need emitting.
            return

        rel = relpath(project.abspath)
        if is_ancestor:
            # If the branch we just left behind is a descendant of
            # the new HEAD (e.g. if this is a topic branch the
            # user is working on and the remote hasn't changed),
            # print a message that makes it easy to get back,
            # no matter where in the workspace os.getcwd() is.
            self.wrn(f'left behind {project.name} branch "{branch}"; '
                     f'to switch back to it (fast forward):\n'
                     f'  git -C {rel} checkout {branch}')
            self.dbg('(To do this automatically in the future,',
                     'use "west update --keep-descendants".)')
        else:
            # Tell the user how they could rebase by hand, and
            # point them at west update --rebase.
            self.wrn(f'left behind {project.name} branch "{branch}"; '
                     f'to rebase onto the new HEAD:\n'
                     f'  git -C {rel} rebase {sha} {branch}')
            self.dbg('(To do this automatically in the future,',
                     'use "west update --rebase".)')

    def ensure_cloned(self, project, stats, take_stats):
        # update() helper. Make sure project is cloned and initialized.

        if take_stats:
            start = perf_counter()
        cloned = project.is_cloned()
        if take_stats:
            stats['check if cloned'] = perf_counter() - start
        if not cloned:
            if take_stats:
                start = perf_counter()
            self.init_project(project)
            if take_stats:
                stats['init'] = perf_counter() - start

    def init_project(self, project):
        # update() helper. Initialize an uncloned project repository.
        # If there's a local clone available, it uses that. Otherwise,
        # it just creates the local repository and sets up the
        # convenience remote without fetching anything from the network.

        cache_dir = self.project_cache(project)

        if cache_dir is None:
            self.small_banner(f'{project.name}: initializing')

            init_cmd = ['init', project.abspath]
            # Silence the very verbose and repetitive init.defaultBranch
            # warning (10 lines per new git clone). The branch
            # 'placeholder' will never have any commit so it will never
            # actually exist.
            if self.git_version_info >= (2, 28, 0):
                init_cmd.insert(1, '--initial-branch=init_placeholder')

            project.git(init_cmd, cwd=self.topdir)

            # This remote is added as a convenience for the user.
            # However, west always fetches project data by URL, not name.
            # The user is therefore free to change the URL of this remote.
            project.git(['remote', 'add', '--', project.remote_name, project.url])
        else:
            self.small_banner(f'{project.name}: cloning from {cache_dir}')
            # Clone the project from a local cache repository. Set the
            # remote name to the value that would be used without a
            # cache.
            project.git(['clone', '--origin', project.remote_name,
                         cache_dir, project.abspath], cwd=self.topdir)
            # Reset the remote's URL to the project's fetch URL.
            project.git(['remote', 'set-url', project.remote_name,
                         project.url])
            # Make sure we have a detached HEAD so we can delete the
            # local branch created by git clone.
            project.git('checkout --quiet --detach HEAD')
            # Find the name of any local branch created by git clone.
            # West commits to only touching 'manifest-rev' in the
            # local branch name space.
            local_branches = project.git(
                ['for-each-ref', '--format', '%(refname)', 'refs/heads/*'],
                capture_stdout=True).stdout.decode('utf-8').splitlines()
            # This should contain at most one branch in current
            # versions of git, but we might as well get them all just
            # in case that changes.
            for branch in local_branches:
                if not branch:
                    continue
                # This is safe: it can't be garbage collected by git before we
                # have a chance to use it, because we have another ref, namely
                # f'refs/remotes/{project.remote_name}/{branch}'.
                project.git(['update-ref', '-d', branch])

    def project_cache(self, project):
        # Find the absolute path to a pre-existing local clone of a project
        # and return it. If the search fails, return None.

        if self.name_cache is not None:
            maybe = Path(self.name_cache) / project.name
            if maybe.is_dir():
                self.dbg(
                    f'found {project.name} in --name-cache {self.name_cache}',
                    level=Verbosity.DBG_MORE)
                return os.fspath(maybe)
            else:
                self.dbg(
                    f'{project.name} not in --name-cache {self.name_cache}',
                    level=Verbosity.DBG_MORE)
        elif self.path_cache is not None:
            maybe = Path(self.path_cache) / project.path
            if maybe.is_dir():
                self.dbg(
                    f'found {project.path} in --path-cache {self.path_cache}',
                    level=Verbosity.DBG_MORE)
                return os.fspath(maybe)
            else:
                self.dbg(
                    f'{project.path} not in --path-cache {self.path_cache}',
                    level=Verbosity.DBG_MORE)

        return None

    def set_new_manifest_rev(self, project, stats, take_stats):
        # update() helper. Make sure project's manifest-rev is set to
        # the latest value it should be.

        if self.fs == 'always' or _rev_type(project) not in ('tag', 'commit'):
            self.fetch(project, stats, take_stats)
        else:
            self.dbg('skipping unnecessary fetch')
            if take_stats:
                start = perf_counter()
            _update_manifest_rev(project, f'{project.revision}^{{commit}}')
            if take_stats:
                stats['set manifest-rev'] = perf_counter() - start

    def fetch(self, project, stats, take_stats):
        # Fetches rev (or project.revision) from project.url in a way that
        # guarantees any branch, tag, or SHA (that's reachable from a
        # branch or a tag) available on project.url is part of what got
        # fetched.
        #
        # Returns a git revision which hopefully can be peeled to the
        # newly-fetched SHA corresponding to rev. "Hopefully" because
        # there are many ways to spell a revision, and they haven't all
        # been extensively tested.

        if take_stats:
            start = perf_counter()

        rev = project.revision

        # Fetch the revision into the local ref space.
        #
        # The following two-step approach avoids a "trying to write
        # non-commit object" error when the revision is an annotated
        # tag. ^{commit} type peeling isn't supported for the <src> in a
        # <src>:<dst> refspec, so we have to do it separately.
        if _maybe_sha(rev) and not self.narrow:
            # We can't in general fetch a SHA from a remote, as some hosts
            # forbid it for security reasons. Let's hope it's reachable
            # from some branch.
            refspec = f'refs/heads/*:{QUAL_REFS}*'
            next_manifest_rev = project.revision
        else:
            # Either the revision is definitely not a SHA and is
            # therefore safe to fetch directly, or the user said
            # that's OK. This avoids fetching unnecessary refs from
            # the remote.
            #
            # We update manifest-rev to FETCH_HEAD instead of using a
            # refspec in case the revision is a tag, which we can't use
            # from a refspec.
            refspec = project.revision
            next_manifest_rev = 'FETCH_HEAD^{commit}'

        self.small_banner(f'{project.name}: fetching, need revision {rev}')
        # --tags is required to get tags if we're not run as 'west
        # update --narrow', since the remote is specified as a URL.
        tags = (['--tags'] if not self.narrow else [])
        clone_depth = (['--depth', str(project.clone_depth)] if
                       project.clone_depth else [])
        # -f is needed to avoid errors in case multiple remotes are
        # present, at least one of which contains refs that can't be
        # fast-forwarded to our local ref space.
        project.git(['fetch', '-f'] + tags + clone_depth +
                    self.args.fetch_opt +
                    ['--', project.url, refspec])

        if take_stats:
            stats['fetch'] = perf_counter() - start

        # Update manifest-rev, leaving an entry in the reflog.
        if take_stats:
            start = perf_counter()

        new_ref = project.sha(next_manifest_rev)
        _update_manifest_rev(project, new_ref)

        if take_stats:
            stats['set manifest-rev'] = perf_counter() - start

    @staticmethod
    def clean_refs_west(project, stats, take_stats):
        # update() helper. Make sure refs/west/* is empty after
        # setting the new manifest-rev.
        #
        # Head of manifest-rev is now pointing to current manifest revision.
        # Thus it is safe to unconditionally clear out the refs/west space.
        #
        # Doing this here instead of in Update.fetch() ensures that it
        # gets cleaned up when users upgrade from older versions of
        # west (like 0.6.x) that didn't handle this properly.
        #
        # In the future, this can be moved into Update.fetch() after
        # the install base of older west versions is expected to be
        # smaller.
        if take_stats:
            start = perf_counter()
        _clean_west_refspace(project)
        if take_stats:
            stats['clean up refs/west/*'] = perf_counter() - start

    @staticmethod
    def ensure_head_ok(project, stats, take_stats):
        # update() helper. Ensure HEAD points at something reasonable.

        if take_stats:
            start = perf_counter()
        head_ok = _head_ok(project)
        if take_stats:
            stats['check HEAD is ok'] = perf_counter() - start
        if not head_ok:
            # If nothing is checked out (which usually only happens if
            # we called Update.init_project() above), check out
            # 'manifest-rev' in a detached HEAD state.
            #
            # Otherwise, it's possible for the initial state to have
            # nothing checked out and HEAD pointing to a non-existent
            # branch. This causes the 'git rev-parse --abbrev-ref HEAD'
            # which happens later in the update to fail.
            #
            # The --detach flag is strictly redundant here, because
            # the qualified manifest-rev form detaches HEAD, but
            # it avoids a spammy detached HEAD warning from Git.
            if take_stats:
                start = perf_counter()
            project.git('checkout --detach ' + QUAL_MANIFEST_REV)
            if take_stats:
                stats['checkout new manifest-rev'] = perf_counter() - start

    def manifest_rev_sha(self, project, stats, take_stats):
        # update() helper. Get the SHA for manifest-rev.

        try:
            if take_stats:
                start = perf_counter()
            return project.sha(QUAL_MANIFEST_REV)
            if take_stats:
                stats['get new manifest-rev SHA'] = perf_counter() - start
        except subprocess.CalledProcessError:
            # This is a sign something's really wrong. Add more help.
            self.err(f'no SHA for branch {MANIFEST_REV} '
                     f'in {project.name_and_path}; was the branch deleted?')
            raise

    def decide_update_strategy(self, project, sha, stats, take_stats):
        # update() helper. Decide on whether we have an ancestor
        # branch or whether we should try to rebase.

        if take_stats:
            start = perf_counter()
        cp = project.git('rev-parse --abbrev-ref HEAD', capture_stdout=True)
        if take_stats:
            stats['get current branch HEAD'] = perf_counter() - start
        current_branch = cp.stdout.decode('utf-8').strip()
        if not current_branch:
            self.die(
                f"Unable to retrieve ref for 'HEAD' in project "
                f"{project.name!r}. It is possible the project has a tag or "
                f"branch with the name 'HEAD'. If so, please delete it.")
        if current_branch != 'HEAD':
            if take_stats:
                start = perf_counter()
            is_ancestor = project.is_ancestor_of(sha, current_branch)
            if take_stats:
                stats['check if HEAD is ancestor of manifest-rev'] = \
                    perf_counter() - start
            try_rebase = self.args.rebase
        else:  # HEAD means no branch is checked out.
            # If no branch is checked out, 'rebase' and
            # 'keep_descendants' don't matter.
            is_ancestor = False
            try_rebase = False

        return current_branch, is_ancestor, try_rebase

    def project_is_active(self, project):
        return self.manifest.is_active(project, extra_filter=self.group_filter)

class ForAll(_ProjectCommand):
    def __init__(self):
        super().__init__(
            'forall',
            'run a command in one or more local projects',
            textwrap.dedent('''\
            Runs a shell (on a Unix OS) or batch (on Windows) command
            within the repository of each of the specified PROJECTs.

            The following variables are set when running your command:
                WEST_PROJECT_NAME
                WEST_PROJECT_PATH
                WEST_PROJECT_ABSPATH (depends on topdir)
                WEST_PROJECT_REVISION
                WEST_PROJECT_URL
                WEST_PROJECT_REMOTE

            Use proper escaping, for example:

                west forall -c "echo \\$WEST_PROJECT_NAME"

            If the command has multiple words, you must quote the -c
            option to prevent the shell from splitting it up. Since
            the command is run through the shell, you can use
            wildcards and the like.

            For example, the following command will list the contents
            of proj-1's and proj-2's repositories on Linux and macOS,
            in long form:

                west forall -c "ls -l" proj-1 proj-2
            '''))

    def do_add_parser(self, parser_adder):
        parser = self._parser(parser_adder,
                              epilog=ACTIVE_CLONED_PROJECTS_HELP)
        parser.add_argument('-c', dest='subcommand', metavar='COMMAND',
                            required=True)
        parser.add_argument('-C', dest='cwd',
                            help='''run commands from this directory;
                            defaults to each project's paths if omitted''')
        parser.add_argument('-a', '--all', action='store_true',
                            help='include inactive projects'),
        parser.add_argument('-g', '--group', dest='groups',
                            default=[], action='append',
                            help='''only run COMMAND if a project is
                            in this group; if given more than once,
                            the command will be run if the project is
                            in any of the groups''')
        parser.add_argument('projects', metavar='PROJECT', nargs='*',
                            help='''projects (by name or path) to operate on;
                            defaults to active cloned projects''')
        return parser

    def do_run(self, args, user_args):
        failed = []
        group_set = set(args.groups)
        env = os.environ.copy()
        for project in self._cloned_projects(args, only_active=not args.all):
            if group_set and not group_set.intersection(set(project.groups)):
                continue

            env["WEST_PROJECT_NAME"] = project.name
            env["WEST_PROJECT_PATH"] = project.path
            env["WEST_PROJECT_ABSPATH"] = project.abspath if project.abspath else ''
            env["WEST_PROJECT_REVISION"] = project.revision
            env["WEST_PROJECT_URL"] = project.url
            env["WEST_PROJECT_REMOTE"] = project.remote_name

            cwd = args.cwd if args.cwd else project.abspath

            self.banner(
                f'running "{args.subcommand}" in {project.name_and_path}:')
            rc = subprocess.Popen(args.subcommand, shell=True, env=env,
                                  cwd=cwd).wait()
            if rc:
                failed.append(project)
        self._handle_failed(args, failed)

GREP_EPILOG = '''
EXAMPLES
--------

To get "git grep foo" results from all cloned, active projects:

  west grep foo

To do the same with:

- git grep --untracked: west grep --untracked foo
- ripgrep:              west grep --tool ripgrep foo
- grep --recursive:     west grep --tool grep foo

To switch the default tool to:

- ripgrep:  west config grep.tool ripgrep
- grep:     west config grep.tool grep

GREP TOOLS
----------

This command runs a "grep tool" command in the top directory of each project.
Supported tools:

- git-grep (default)
- ripgrep
- grep

Set the "grep.tool" configuration option to change the default.

Use "--tool" to switch the tool from its default.

TOOL PATH
---------

Use --tool-path to override the path to the tool. For example:

  west grep --tool ripgrep --tool-path /my/special/ripgrep

Without --tool-path, the "grep.<TOOL>-path" configuration option
is checked next. For example, to set the default path to ripgrep:

  west config grep.ripgrep-path /my/special/ripgrep

If the option is not set, "west grep" searches for the tool as follows:

- git-grep: search for "git" (and run as "git grep")
- ripgrep: search for "rg", then "ripgrep"
- grep: search for "grep"

TOOL ARGUMENTS
--------------

The "grep.<TOOL>-args" configuration options, if set, contain arguments
that are always passed to the tool. The defaults for these are:

- git-grep: (none)
- ripgrep: (none)
- grep: "--recursive"

Command line options or arguments not recognized by "west grep" are
passed to the tool after that. This applies to --foo here, for example:

  west grep --foo --project=myproject

To force arguments to be given to the tool instead of west, put them
after a "--", like the --project and --tool-path options here:

  west grep -- --project=myproject --tool-path=mypath

Arguments before '--' that aren't recognized by west grep are still
passed to the grep tool.

To pass '--' to the grep tool, pass one for 'west grep' first.
For example, to search for '--foo' with grep, you can run:

  west grep --tool grep -- -- --foo

The first '--' separates west grep args from tool args. The second '--'
separates options from positional arguments in the 'grep' command line.

COLORS
------

By default, west will force the tool to print colored output as long
as the "color.ui" configuration option is true. If color.ui is false,
west forces the tool not to print colored output.

Since all supported tools have similar --color options, you can
override this behavior on the command line, for example with:

  west grep --color=never

To do this permanently, set "grep.color":

  west config grep.color never
'''
# color.ui limitation:
# https://github.com/zephyrproject-rtos/west/issues/651

class Grep(_ProjectCommand):

    # When adding a tool, make sure to check:
    #
    # - the 'failed' handling below for proper exit codes
    # - color handling works as expected
    TOOLS = ['git-grep', 'ripgrep', 'grep']

    DEFAULT_TOOL = 'git-grep'

    DEFAULT_TOOL_ARGS = {
        'git-grep': [],
        'ripgrep': [],
        'grep': ['--recursive'],
    }

    DEFAULT_TOOL_EXECUTABLES = {
        # git-grep is intentionally omitted: use self._git
        'ripgrep': ['rg', 'ripgrep'],
        'grep': ['grep'],
    }

    def __init__(self):
        super().__init__(
            'grep',
            'run grep or a grep-like tool in one or more local projects',
            'Run grep or a grep-like tool in one or more local projects.',
            accepts_unknown_args=True)

    def do_add_parser(self, parser_adder):
        parser = self._parser(parser_adder, epilog=GREP_EPILOG)
        parser.add_argument('--tool', choices=self.TOOLS, help='grep tool')
        parser.add_argument('--tool-path', help='path to grep tool executable')
        # Unlike other project commands, we don't take the project as
        # a positional argument. This inconsistency makes the usual
        # use case of "search the entire workspace" faster to type.
        parser.add_argument('-p', '--project', metavar='PROJECT',
                            dest='projects', default=[], action='append',
                            help='''project to run grep tool in (may be given
                            more than once); default is all cloned active
                            projects''')
        return parser

    def do_run(self, args, tool_cmdline_args):
        tool = self.tool(args)
        command_list = ([self.tool_path(tool, args)] +
                        self.tool_args(tool, tool_cmdline_args))
        failed = []
        for project in self._cloned_projects(args,
                                             only_active=not args.projects):
            completed_process = self.run_subprocess(
                command_list,
                capture_output=True,
                text=True,
                cwd=project.abspath)

            # By default, supported tools generally exit 1 if
            # nothing is found and 0 if something was found, with
            # other return codes indicating an error.
            #
            # For git grep, this is experimentally true, even if
            # the man page doesn't say so.
            #
            # Per grep's and ripgrep's man pages, this is true
            # except that if an error occurred and --quiet was
            # given and a match was found, then the exit status is
            # still 0.
            #
            # Using --quiet can thus be used to print repositories
            # with a match, while ignoring errors.
            if completed_process.returncode == 1:
                continue
            self.banner(f'{project.name_and_path}:')
            if completed_process.returncode != 0:
                self.err(f'{util.quote_sh_list(command_list)}:\n'
                         f'{completed_process.stderr}', end='')
                failed.append(project)
            elif completed_process.stdout:
                # With 'west grep --quiet PATTERN', we will just be
                # printing a list of repositories that contain
                # matches. We want to avoid printing the newline from
                # self.inf() in that case.
                self.inf(completed_process.stdout, end='')
        self._handle_failed(args, failed)

    def tool(self, args):
        if args.tool:
            return args.tool

        return self.config.get('grep.tool', self.DEFAULT_TOOL)

    def tool_path(self, tool, args):
        if args.tool_path:
            return args.tool_path

        config_option = self.config.get(f'grep.{tool}-path')
        if config_option:
            return config_option

        if tool == 'git-grep':
            self.die_if_no_git()
            return self._git

        for executable in self.DEFAULT_TOOL_EXECUTABLES[tool]:
            path = shutil.which(executable)
            if path is not None:
                return path

        self.die(f'grep tool "{tool}" not found, please use --tool-path')

    def tool_args(self, tool, cmdline_args):
        ret = []

        if tool == 'git-grep':
            ret.append('grep')

        config_color = self.config.get('grep.color')
        if config_color:
            color = config_color
        else:
            if self.color_ui:
                color = 'always'
            else:
                color = 'never'

        ret.extend([f'--color={color}'])
        ret.extend(shlex.split(self.config.get(f'grep.{tool}-args', '')) or
                   self.DEFAULT_TOOL_ARGS[tool])

        # The first '--' we see is "meant for" west grep. Take that
        # out of the options we pass to the tool.
        found_first_dashes = False
        for arg in cmdline_args:
            if arg == '--' and not found_first_dashes:
                found_first_dashes = True
            else:
                ret.append(arg)

        return ret

class Topdir(_ProjectCommand):
    def __init__(self):
        super().__init__(
            'topdir',
            'print the top level directory of the workspace',
            textwrap.dedent('''\
            Prints the absolute path of the current west workspace's
            top directory.

            This is the directory containing .west. All project
            paths in the manifest are relative to this top directory.'''))

    def do_add_parser(self, parser_adder):
        return self._parser(parser_adder)

    def do_run(self, args, user_args):
        self.inf(PurePath(self.topdir).as_posix())

class SelfUpdate(_ProjectCommand):
    def __init__(self):
        super().__init__(
            'selfupdate',
            'deprecated; exists for backwards compatibility',
            'Do not use. You can upgrade west with pip only from v0.6.0.')

    def do_add_parser(self, parser_adder):
        return self._parser(parser_adder)

    def do_run(self, args, user_args):
        self.die(self.description)

#
# Private helper routines.
#

def _clean_west_refspace(project):
    # Clean the refs/west space to ensure they do not show up in 'git log'.

    # Get all the ref names that start with refs/west/.
    list_refs_cmd = ['for-each-ref', '--format=%(refname)', '--',
                     QUAL_REFS + '**']
    cp = project.git(list_refs_cmd, capture_stdout=True)
    west_references = cp.stdout.decode('utf-8').strip()

    # Safely delete each one.
    for ref in west_references.splitlines():
        delete_ref_cmd = ['update-ref', '-d', ref]
        project.git(delete_ref_cmd)

def _update_manifest_rev(project, new_manifest_rev):
    project.git(['update-ref',
                 '-m', f'west update: moving to {new_manifest_rev}',
                 QUAL_MANIFEST_REV, new_manifest_rev])

def _maybe_sha(rev):
    # Return true if and only if the given revision might be a SHA.

    try:
        int(rev, 16)
    except ValueError:
        return False

    return len(rev) <= 40

def _rev_type(project, rev=None):
    # Returns a "refined" revision type of rev (default:
    # project.revision) as one of the following strings: 'tag', 'tree',
    # 'blob', 'commit', 'branch', 'other'.
    #
    # The approach combines git cat-file -t and git rev-parse because,
    # while cat-file can for sure tell us a blob, tree, or tag, it
    # doesn't have a way to disambiguate between branch names and
    # other types of commit-ishes, like SHAs, things like "HEAD" or
    # "HEAD~2", etc.
    #
    # We need this extra layer of refinement to be able to avoid
    # fetching SHAs that are already available locally.
    #
    # This doesn't belong in manifest.py because it contains "west
    # update" specific logic.
    if not rev:
        rev = project.revision
    cp = project.git(['cat-file', '-t', rev], check=False,
                     capture_stdout=True, capture_stderr=True)
    stdout = cp.stdout.decode('utf-8').strip()
    if cp.returncode:
        return 'other'
    elif stdout in ('blob', 'tree', 'tag'):
        return stdout
    elif stdout != 'commit':    # just future-proofing
        return 'other'

    # to tell branches, lightweight tags, and commits apart, we need rev-parse.
    cp = project.git(['rev-parse', '--verify', '--symbolic-full-name', rev],
                     check=False, capture_stdout=True, capture_stderr=True)
    if cp.returncode:
        # This can happen if the ref name is ambiguous, e.g.:
        #
        # $ git update-ref ambiguous-ref HEAD~2
        # $ git checkout -B ambiguous-ref
        #
        # Which creates both .git/ambiguous-ref and
        # .git/refs/heads/ambiguous-ref.
        return 'other'

    stdout = cp.stdout.decode('utf-8').strip()
    if stdout.startswith('refs/heads'):
        return 'branch'
    elif stdout.startswith('refs/tags'):
        # Annotated tags are handled above. Lightweight tags are
        # handled here.
        return 'tag'
    elif not stdout:
        return 'commit'
    else:
        return 'other'

def _head_ok(project):
    # Returns True if the reference 'HEAD' exists and is not a tag or remote
    # ref (e.g. refs/remotes/origin/HEAD).
    # Some versions of git will report 1, when doing
    # 'git show-ref --verify HEAD' even if HEAD is valid, see #119.
    # 'git show-ref --head <reference>' will always return 0 if HEAD or
    # <reference> is valid.
    # We are only interested in HEAD, thus we must avoid <reference> being
    # valid. '/' can never point to valid reference, thus 'show-ref --head /'
    # will return:
    # - 0 if HEAD is present
    # - 1 otherwise
    return project.git('show-ref --quiet --head /',
                       check=False).returncode == 0

def projects_unknown(manifest, projects):
    # Retrieve the projects with get_projects(project,
    # only_cloned=False). Return a pair: (projects, unknown)
    # containing either a projects list and None or None and a list of
    # unknown project IDs.

    try:
        return (manifest.get_projects(projects, only_cloned=False), None)
    except ValueError as ve:
        if len(ve.args) != 2:
            raise          # not directly raised by get_projects()
        unknown = ve.args[0]
        if not unknown:
            raise   # only_cloned is False, so this "can't happen"
        return (None, unknown)

#
# Special files and directories in the west workspace.
#
# These are given variable names for clarity, but they can't be
# changed without propagating the changes into west itself.
#

# Top-level west directory, containing west itself and the manifest.
WEST_DIR = util.WEST_DIR

# Default manifest repository URL.
MANIFEST_URL_DEFAULT = 'https://github.com/zephyrproject-rtos/zephyr'

#
# Other shared globals.
#

ACTIVE_PROJECTS_HELP = '''\
ACTIVE PROJECTS
---------------

Default output is limited to "active" projects as determined by the:

- "group-filter" manifest file section
- "manifest.group-filter" local configuration option in .west/config

To include inactive projects as well, use "--all" or give an explicit
list of projects (by name or path). See the west documentation for
more details on active projects.
'''

ACTIVE_CLONED_PROJECTS_HELP = f'''\
{ACTIVE_PROJECTS_HELP}

Regardless of the above, output is limited to cloned projects.
'''

#
# Helper class for creating format string keys that are expensive or
# undesirable to compute if not needed.
#

class DelayFormat:
    '''Delays formatting an object.'''

    def __init__(self, obj):
        '''Delay formatting `obj` until a format operation using it.

        :param obj: object to format

        If callable(obj) returns True, then obj() will be used as the
        string to be formatted. Otherwise, str(obj) is used.'''
        self.obj = obj
        self.as_str = None

    def __format__(self, format_spec):
        if self.as_str is None:
            if callable(self.obj):
                self.as_str = self.obj()
                assert isinstance(self.as_str, str)
            else:
                self.as_str = str(self.obj)
        return ('{:' + format_spec + '}').format(self.as_str)

#
# Logging helpers
#


class ProjectCommandLogFormatter(logging.Formatter):

    def __init__(self):
        super().__init__(fmt='%(name)s: %(message)s')


class ProjectCommandLogHandler(logging.Handler):

    def __init__(self, command):
        super().__init__()
        self.command = command
        self.setFormatter(ProjectCommandLogFormatter())

    def emit(self, record):
        fmt = self.format(record)
        lvl = record.levelno
        if lvl > logging.CRITICAL:
            self.command.die(fmt)
        elif lvl >= logging.ERROR:
            self.command.err(fmt)
        elif lvl >= logging.WARNING:
            self.command.wrn(fmt)
        elif lvl >= logging.INFO:
            self.command.inf(fmt)
        elif lvl >= logging.DEBUG:
            self.command.dbg(fmt)
        else:
            self.command.dbg(fmt, level=Verbosity.DBG_EXTREME)
