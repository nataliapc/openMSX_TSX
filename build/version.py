# Contains the openMSX version number and versioning related functions.

from executils import captureStdout
from makeutils import filterLines

from io import open
from os import makedirs
from os.path import isdir
import re

# Name used for packaging.
packageName = 'openmsx'

# Version number.
packageVersionNumber = '16.0'

# Version code for Android must be an incremental number
# Increase this number for each release build. For a dev build, the
# version number is based on the git commit count but for a release
# build, it must be hardcoded
androidReleaseVersionCode=11

# Note: suffix should be empty or with dash, like "-rc1" or "-test1"
packageVersionSuffix = '-rc1'
packageVersion = packageVersionNumber + packageVersionSuffix

# Is this a release version ("True") or development version ("False").
releaseFlag = True

def _extractRevisionFromStdout(log, command, regex):
	text = captureStdout(log, command)
	if text is None:
		# Error logging already done by captureStdout().
		return None
	# pylint 0.18.0 somehow thinks captureStdout() returns a list, not a string.
	lines = text.split('\n') # pylint: disable-msg=E1103
	for revision, in filterLines(lines, regex):
		print('Revision number found by "%s": %s' % (command, revision), file=log)
		return revision
	else:
		print('Revision number not found in "%s" output:' % command, file=log)
		print(str(text), file=log)
		return None

def extractGitRevision(log):
	return _extractRevisionFromStdout(
		log, 'git describe --dirty', r'\S+?-(\S+)$'
		)

def extractNumberFromGitRevision(revisionStr):
	if revisionStr is None:
		return None
	if revisionStr == 'dirty':
		return None
	return re.match(r'(\d+)+', revisionStr).group(0)

_cachedRevision = False # because None is a valid result

def extractRevision():
	global _cachedRevision
	if _cachedRevision is not False:
		return _cachedRevision
	if releaseFlag:
		# Not necessary, we do not append revision for a release build.
		return None
	if not isdir('derived'):
		makedirs('derived')
	with open('derived/version.log', 'w', encoding='utf-8') as log:
		print('Extracting revision info...', file=log)
		revision = extractGitRevision(log)
		print('Revision string: %s' % revision, file=log)
		revisionNumber = extractNumberFromGitRevision(revision)
		print('Revision number: %s' % revisionNumber, file=log)
	_cachedRevision = revision
	return revision

def extractRevisionNumber():
	return int(extractNumberFromGitRevision(extractRevision()) or 0)

def extractRevisionString():
	return extractRevision() or 'unknown'

def getDetailedVersion():
	if releaseFlag:
		return packageVersion
	else:
		return '%s-%s' % (packageVersion, extractRevisionString())

def getVersionedPackageName():
	return '%s-%s' % (packageName, getDetailedVersion())

def countGitCommits():
	if not isdir('derived'):
		makedirs('derived')
	with open('derived/commitCountVersion.log', 'w', encoding='utf-8') as log:
		print('Extracting commit count...', file=log)
		commitCount = captureStdout(log, 'git rev-list HEAD --count')
		print('Commit count: %s' % commitCount, file=log)
	return commitCount

def getAndroidVersionCode():
	if releaseFlag:
		return '%s' % (androidReleaseVersionCode)
	else:
		return '%s' % ( countGitCommits() )

if __name__ == '__main__':
	print(packageVersionNumber)
