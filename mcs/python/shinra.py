#!/usr/bin/env python3
# Copyright 2015 Shinra Technologies Inc.

import logging
import json
import hashlib
import os
import os.path
import re
import shutil
import stat
import sys
import argparse
import zipfile
import tarfile
import socket
import struct
import time
import shlex
import subprocess

logger = logging.getLogger(__name__)

CONF_FILE_NAME = 'Configuration.json'
MANIFEST_FILE_NAME = 'Manifest.txt'

default_settings = {
	'StorePath': 'C:\\Shinra\\Local',
	'ArchivePath': 'C:\\Shinra\\UserFiles',
	'GamesInstallDir': 'C:\\Shinra\\Games',
	'DisplayStatistics': False,
	'EncoderThreads': 2,
	'MaxEncodingFrames': 5,
	'MaxRenderingFrames': 3,
	'ShowWindow': False,
	# By default, shinra.py is installed in MCS_PATH/python
	'MCSPath' : os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
	'OverwriteGameInstall' : False,
	'CleanupOnInstall' : False,
}

userDir = os.environ.get('APPDATA')
if not userDir:
	userDir = os.environ.get('HOME')
if not userDir:
	userDir = "."
configurationFile = "%s\\Shinra\\MCS\\Settings.json" % userDir

MCS_DLL_MAPPING = {
	'D3D9.dll': 'ShimD3D9.dll',
	'D3D11.dll': 'ShimD3D11.dll',
	'DXGI.dll': 'ShimDXGI.dll',
	'XInput1_3.dll': 'ShimXInput.dll',
	'XInput1_4.dll': 'ShimXInput.dll',
	'XInput9_1_0.dll': 'ShimXInput.dll',
	'CloudCoreClient.dll': 'CloudCoreClient.dll',
	'd3dcompiler_47.dll': 'd3dcompiler_47.dll',
}

def getMCSSettings():
	logger.debug("Loading MCS configuration %s", configurationFile)
	conf = None
	if os.path.exists(configurationFile):
		try:
			with open(configurationFile, "r") as f:
				conf = json.load(f)
		except:
			logger.exception("Failed to parse configuration file %s.", configurationFile)
	if conf:
		# if there are some missing fields in the user's conf, make sure we get the default value.
		tmp = default_settings.copy()
		tmp.update(conf)
		conf = tmp
	else:
		conf = default_settings
	return conf

def setMCSSettings(conf):
	prevConf = getMCSSettings()
	logger.debug("Setting MCS configuration: %s", conf)
	prevConf.update(conf)
	confDir = os.path.dirname(configurationFile)
	if not os.path.exists(confDir):
		os.makedirs(confDir)
	with open(configurationFile, 'w') as f:
		json.dump(prevConf, f, sort_keys=True, indent=4)

def validateMCSSettings(conf):
	if 'StorePath' in conf:
		if not os.path.isdir(conf['StorePath']):
			raise RuntimeError("StorePath must be an existing directory.")
	if 'ArchivePath' in conf:
		if not os.path.isdir(conf['ArchivePath']):
			raise RuntimeError("ArchivePath must be an existing directory.")
	if 'GamesInstallDir' in conf:
		if not os.path.isdir(conf['GamesInstallDir']):
			raise RuntimeError("GamesInstallDir must be an existing directory.")
	if 'MCSPath' in conf:
		if not os.path.isdir(conf['MCSPath']):
			raise RuntimeError("MCSPath must be an existing directory.")
		checkMCSPath(conf['MCSPath'])
	if 'ShowWindow' in conf:
		if not isinstance(conf['ShowWindow'], bool):
			raise RuntimeError("ShowWindow must be an boolean value.")
	if 'DisplayStatistics' in conf:
		logger.debug("DisplayStatistics is %s", conf['DisplayStatistics'])
		if not isinstance(conf['DisplayStatistics'], bool):
			raise RuntimeError("DisplayStatistics must be an boolean value.")
	if 'EncoderThreads' in conf:
		val = conf['EncoderThreads']
		if not isinstance(val, int):
			raise RuntimeError("EncoderThreads must be an integer value.")
		if val < 1 or val > 10:
			raise RuntimeError("EncoderThreads must be between 1 and 10.")
	if 'MaxEncodingFrames' in conf:
		val = conf['MaxEncodingFrames']
		if not isinstance(val, int):
			raise RuntimeError("MaxEncodingFrames must be an integer value.")
		if val < 1 or val > 10:
			raise RuntimeError("MaxEncodingFrames must be between 1 and 10.")
	if 'MaxRenderingFrames' in conf:
		val = conf['MaxRenderingFrames']
		if not isinstance(val, int):
			raise RuntimeError("MaxRenderingFrames must be an integer value.")
		if val < 1 or val > 10:
			raise RuntimeError("MaxRenderingFrames must be between 1 and 10.")
	if 'OverwriteGameInstall' in conf:
		if not isinstance(conf['OverwriteGameInstall'], bool):
			raise RuntimeError("OverwriteGameInstall must be an boolean value.")
	if 'CleanupOnInstall' in conf:
		if not isinstance(conf['CleanupOnInstall'], bool):
			raise RuntimeError("CleanupOnInstall must be an boolean value.")


def getFileHash(f, blockSizeBytes = 1024 * 1024 * 4):
	h = hashlib.sha1()
	while True:
		tmp = f.read(blockSizeBytes)
		if not tmp:
			break
		h.update(tmp)
	return h.hexdigest()

def getStringHash(data):
	h = hashlib.sha1()
	h.update(data)
	return h.hexdigest()

def zipHashJson(z, obj, path):
	s = json.dumps(obj, sort_keys=True, indent=4).encode('utf-8')
	z.writestr(path, s)
	return getStringHash(s)

def zipManiefst(z, manifest):
	s = ""
	for path, hash in manifest.items():
		# ensure Unix style directory separator. Python on Windows can work with "/"
		path = path.replace("\\", "/")
		s += "{0} {1}\n".format(hash, path)
	z.writestr(MANIFEST_FILE_NAME, s)

def make_tarfile(output_filename, source_dir):
	with tarfile.open(output_filename, "w:gz") as tar:
		tar.add(source_dir, arcname=os.path.basename(source_dir))
	return output_filename

def isChildPath(parent, child):
	parentPath = re.split("[\\/]", parent)
	childPath = re.split("[\\/]", child)
	if len(parentPath) > len(childPath):
		return False
	while len(parentPath) > 0:
		if (parentPath[0] != childPath[0]):
			return False
		parentPath.pop(0)
		childPath.pop(0)
	return True

def _getFilesFromDataPack(datapack):
	all_files = {}
	files = datapack.get('files')
	if files and len(files) > 0:
		mapFsysPath = files[0].get('fileSystemPath')
		mapAliasPath = files[0].get('aliasPath')
		if os.path.isdir(mapFsysPath):
			for root, dirs, files in os.walk(mapFsysPath):
				for f in files:
					filePath = os.path.join(root, f)
					aliasPath = os.path.relpath(filePath, mapFsysPath)
					aliasPath = os.path.join(mapAliasPath, aliasPath)
					all_files[aliasPath] = filePath
		elif os.path.isfile(mapFsysPath):
			filePath = mapFsysPath
			aliasPath = os.path.relpath(filePath, mapFsysPath)
			aliasPath = os.path.join(mapAliasPath, aliasPath)
			all_files[aliasPath] = filePath
		else:
			raise RuntimeError("File mapping in data pack {0} refers to invalid path: {1}".format(datapack['id'], mapFsysPath))
	return all_files

def _getDataPackSize(datapack):
	totalsize = 0
	for alias, file in _getFilesFromDataPack(datapack).items():
		if os.path.exists(file):
			totalsize += os.path.getsize(file)
	return totalsize

def _resolveDataPackAlias(datapack, alias):
	datapackid = datapack['id']
	files = datapack.get('files')
	if files and len(files) > 0:
		mapFsysPath = files[0].get('fileSystemPath')
		mapAliasPath = files[0].get('aliasPath')
	if not mapFsysPath:
		raise RuntimeError("File mapping in data pack {0} has empty path".format(datapackid))
	if not os.path.isdir(mapFsysPath):
		raise RuntimeError("File mapping in data pack {0} refers to invalid path: {1}".format(datapackid, mapFsysPath))
	aliasRelPath = os.path.relpath(alias, mapAliasPath)
	return os.path.join(mapFsysPath, aliasRelPath)

def checkMCSPath(mcsPath):
	mcsDir = os.path.join(mcsPath, 'Shinra', 'x32', 'Release')
	for linkName, dllName in MCS_DLL_MAPPING.items():
		linkTarget = os.path.join(mcsDir, dllName)
		if not os.path.exists(linkTarget):
			relPath = os.path.relpath(linkTarget, mcsPath)
			raise RuntimeError("Directory {0} does not contain MCS dll {1}.".format(mcsPath, relPath))
	mcsDir = os.path.join(mcsPath, 'Shinra', 'x64', 'Release')
	for linkName, dllName in MCS_DLL_MAPPING.items():
		linkTarget = os.path.join(mcsDir, dllName)
		if not os.path.exists(linkTarget):
			relPath = os.path.relpath(linkTarget, mcsPath)
			raise RuntimeError("Directory {0} does not contain MCS dll {1}.".format(mcsPath, relPath))

def checkDllLink(linkPath, linkTarget):
	if os.path.exists(linkPath):
		os.chmod(linkPath, stat.S_IWRITE )
		os.remove(linkPath)
	try:
		os.symlink(linkTarget, linkPath)
	except:
		logger.exception("Failed to create link %s to %s. Copy Dll instead.", linkPath, linkTarget)
		shutil.copy(linkTarget, linkPath)

def remove_readonly(func, path, exc_info):
	"""
	Error handler for ``shutil.rmtree``.

	If the error is due to an access error (read only file)
	it attempts to add write permission and then retries.

	If the error is for another reason it re-raises the error.

	Usage : ``shutil.rmtree(path, onerror=remove_readonly)``
	"""
	import stat
	# logging.error("rmtree failed with {0} and path {1}: {2}".format(func, path, exc_info))
	if func is os.rmdir:
		os.chmod(path, stat.S_IWRITE)
		os.rmdir(path)
	elif func is os.remove:
		if os.path.isdir(path):
			os.chmod(path, stat.S_IWRITE)
			os.remove(path)
	else:
		raise

def remove_dir(path):
	# try multiple times in case it's locked by win explorer
	logging.debug("Removing directory {0}".format(path))
	for i in range(69):
		try:
			if os.path.isdir(path):
				shutil.rmtree(path, onerror=remove_readonly)
			return
			
		except WindowsError as e:
			if e.winerror != 145:
				raise e
				
	raise Exception("Failed to delete {0}.".format(path))

class progressreport:
	def progressInit(self, totalSize):
		self.increment = totalSize//100
		self.percent = 0
		self.current = 0

	def progressUpdate(self, filesize):
		if self.increment:
			self.current += filesize;
			if self.current > self.increment:
				self.percent += self.current // self.increment
				self.current = self.current % self.increment
				logger.info("Progression: %d%%", self.percent)

class project:
	"""Manages a Shinra project"""
	
	def __init__(self, projectfile):
		self.projectfile = projectfile
		with open(projectfile, 'r') as f:
			self.conf = json.load(f)
	
	def _startups(self):
		c = self.conf
		startups = c.get("startups")
		if not startups:
			raise RuntimeError("Project {0} does not contain startups section.".format(self.projectfile))
		return startups
	
	def _datapacks(self):
		c = self.conf
		if not "dataPacks" in c:
			raise RuntimeError("Project {0} does not contain dataPacks section".format(self.projectfile))
		dataPacks = c["dataPacks"]
		if not dataPacks:
			raise RuntimeError("Project {0} contains empty dataPacks section.".format(self.projectfile))
		return dataPacks
	
	def _findstartup(self, id):
		startups = self._startups()
		for s in startups:
			startupId = s.get('id')
			if not startupId:
				raise RuntimeError("Project {0} contain statup configuration without an id.".format(self.projectfile))
			if startupId == id:
				return s
		raise RuntimeError("Project {0} does not contain startup {1}.".format(self.projectfile, id))
	
	def _finddatapack(self, id):
		dataPacks = self._datapacks()
		for d in dataPacks:
			dataPackId = d.get('id')
			if not dataPackId:
				raise RuntimeError("Project {0} contain datapack without an id.".format(self.projectfile))
			if dataPackId == id:
				return d
		return None
	
	def getstartup(self, id):
		s = self._findstartup(id)
		executable = s.get('executable')
		if not executable:
			raise RuntimeError("Project {0} contains startup {1} without an executable property".format(self.projectfile, id))
		if not 'workDir' in s:
			raise RuntimeError("Project {0} contains startup {1} without an workDir property".format(self.projectfile, id))
		workDir = s['workDir']
		if not 'arguments' in s:
			raise RuntimeError("Project {0} contains startup {1} without an arguments property".format(self.projectfile, id))
		arguments = s['arguments']
		dataPackId = s.get('dataPackId')
		if not dataPackId:
			raise RuntimeError("Project {0} contains startup {1} without a dataPackId property".format(self.projectfile, id))
		dataPack = self._finddatapack(dataPackId)
		if not dataPack:
			raise RuntimeError("Project {0} contains startup {1} refering to unknown dataPack {2}".format(self.projectfile, id, dataPackId))
		if workDir:
			dirPath = _resolveDataPackAlias(dataPack, workDir)
			if not os.path.isdir(dirPath):
				raise RuntimeError("Project {0} contains startup {1} with workDir pointing to invalid directory {2} in dataPack {3}: {4}".format(self.projectfile, id, workDir, dataPackId, dirPath))
		exePath = _resolveDataPackAlias(dataPack, executable)
		if not os.path.isfile(exePath):
			raise RuntimeError("Project {0} contains startup {1} with executable pointing to invalid file {2} in dataPack {3}: {4}".format(self.projectfile, id, executable, dataPackId, exePath))
		return s, dataPack
	
	def projectId(self):
		c = self.conf
		name = c.get('projectId')
		if not name:
			raise RuntimeError("Project {0} has empty projectId property".format(self.projectfile))
		return name
	
	def version(self):
		c = self.conf
		if not 'projectVersion' in c:
			raise RuntimeError("Project {0} does not contain projectVersion property".format(self.projectfile))
		return c['projectVersion']
	
	def getTotalFilesSize(self):
		totalsize = 0
		datapacks = {}
		startups = self.conf.get('startups')
		if startups:
			for startup in startups:
				startupid = startup.get('id')
				if not startup:
					raise RuntimeError("Project {0} has startup configuration with no id.".format(self.projectfile))
				dataPackId = startup.get('dataPackId')
				if not dataPackId:
					raise RuntimeError("Project {0} has startup configuration {0} with no datapack configuration.".format(self.projectfile, startupid))
				p = self._finddatapack(dataPackId)
				if not p:
					raise RuntimeError("Project {0} has startup configuration {0} referring to non existing datapack {1}.".format(self.projectfile, startupid, dataPackId))
				datapacks[dataPackId] = p
		for dpid, datapack in datapacks.items():
			totalsize += _getDataPackSize(datapack)
		return totalsize
	
	def getStartupFilesSize(self, startupId):
		totalsize = 0
		datapacks = {}
		startup, datapack = self.getstartup(startupId)
		return _getDataPackSize(datapack)

class package(progressreport):
	"""Manages a ShinraPackage"""
	
	def __init__(self, archive):
		logger.debug('New ShinraPack: %s', archive)
		self.archive = archive
		self.increment = None
	
	def compress(self, project):
		datapacks = {}
		projectId = project.projectId()
		conf = { 'id': projectId, 'version': project.version() , 'startups' : [], 'server_packs' : [] }
		for startup in project._startups():
			id = startup.get('id')
			if not id:
				raise RuntimeError("Project {0} has startup configuration with empty id.".format(projectId))
			startup, dataPack = project.getstartup(id)
			datapacks[startup['dataPackId']] = dataPack
			conf['startups'].append(startup)
		for dpid, datapack in datapacks.items():
			srvPack = { 'id': dpid, 'version': datapack['version'] }
			conf['server_packs'].append(srvPack)
		manifest = {}
		with zipfile.ZipFile(self.archive, 'w', allowZip64=True) as z:
			manifest[CONF_FILE_NAME] = zipHashJson(z, conf, CONF_FILE_NAME)
			for dpid, datapack in datapacks.items():
				logger.debug("Zipping DataPack %s", dpid)
				for alias, file in _getFilesFromDataPack(datapack).items():
					if not os.path.exists(file):
						raise RuntimeError("File {0} cannot be found or link cannot be resolved.".format(file))
					else:
						alias = os.path.join(dpid, alias)
						logger.debug("Compute hash for %s", file)
						with open(file, "rb") as f:
							manifest[alias] = getFileHash(f)
						logger.debug("Compress file %s", file)
						z.write(file, alias)
						self.progressUpdate(os.path.getsize(file))
			zipManiefst(z, manifest)
	
	def validate(self):
		isValid = True
		manifestline = re.compile("([0-9a-fA-F]+)\s+(.*)")
		with zipfile.ZipFile(self.archive, 'r', allowZip64=True) as z:
			manifest = {}
			with z.open(MANIFEST_FILE_NAME) as mdata:
				for l in mdata:
					l = l.decode("utf-8")
					match = manifestline.match(l)
					if match:
						hash = match.group(1)
						fileName = match.group(2)
						manifest[fileName] = hash
					else:
						logger.error('failed to parse manifest line: %s', l)
			for file, hash in manifest.items():
				finfo = z.getinfo(file)
				with z.open(finfo, 'r') as f:
					h = getFileHash(f)
				if h != hash:
					logger.error("Non matching hash for %s. Expected %s got %s", file, hash, h)
					isValid = False
				else:
					logger.debug("Hash OK for %s. Expected %s got %s", file, hash, h)
				self.progressUpdate(finfo.file_size)
		return isValid
	
	def uncompress(self, path):
		with zipfile.ZipFile(self.archive, "r") as z:
			with z.open(CONF_FILE_NAME) as confData:
				confData = confData.read().decode("utf-8")
				conf = json.loads(confData)
			if (not 'server_packs' in conf) or (not conf['server_packs']):
				raise RuntimeError("Configuration in {0} does not contain any server_packs".format(self.archive))
			for srvPack in conf['server_packs']:
				if (not 'id' in srvPack) or (not srvPack['id']):
					raise RuntimeError("Configuration in {0} contains a server_pack with no id defined".format(self.archive))
				srvPackId = srvPack['id']
				srvPackPath = os.path.join(path, srvPackId)
				for finfo in z.infolist():
					if (isChildPath(srvPackId, finfo.filename)):
						logger.debug("Extract %s to %s", finfo.filename, path)
						z.extract(finfo, path)
						self.progressUpdate(finfo.file_size)
	
	def createproject(self, project, path):
		with zipfile.ZipFile(self.archive, "r") as z:
			with z.open(CONF_FILE_NAME) as confData:
				confData = confData.read().decode("utf-8")
				conf = json.loads(confData)
			packId = conf.get('id')
			if not packId:
				raise RuntimeError("Configuration in {0} does not contain package id.".format(self.archive))
			if (not 'version' in conf):
				raise RuntimeError("Configuration in {0} does not contain version.".format(self.archive))
			proj = { 'projectId': packId, 'projectVersion': conf['version'],
					 'startups': [], 'dataPacks': []
					}
			server_packs = conf.get('server_packs')
			if not server_packs:
				raise RuntimeError("Configuration in {0} does no contain server_packs.".format(self.archive))
			for srvPackConf in server_packs:
				srvPackId = srvPackConf.get('id')
				if not srvPackId:
					raise RuntimeError("Configuration in {0} contains a server_pack with no id defined.".format(self.archive))
				if (not 'version' in srvPackConf):
					raise RuntimeError("Configuration in {0} contains a server_pack with no version defined.".format(self.archive))
				
				srvPackPath = os.path.join(path, srvPackId)
				datapack = { 'id': srvPackId,
							 'version': srvPackConf['version'],
							 'files' : [
								{
									'aliasPath': '',
									'fileSystemPath': srvPackPath
								}
							 ]
							}
				proj['dataPacks'].append(datapack)
			startups = conf.get('startups')
			if not startups:
				raise RuntimeError("Configuration in {0} does no contain startups.".format(self.archive))
			for startupcfg in startups:
				proj['startups'].append(startupcfg)
			with open(project, 'w') as f:
				json.dump(proj, f, sort_keys=True, indent=4)
	
	def getConf(self):
		with zipfile.ZipFile(self.archive, "r") as z:
			with z.open(CONF_FILE_NAME) as confData:
				confData = confData.read().decode("utf-8")
				return json.loads(confData)
	
	def getServerConf(self, serverId):
		with zipfile.ZipFile(self.archive, "r") as z:
			with z.open(CONF_FILE_NAME) as confData:
				srvPackData = confData.read().decode("utf-8")
				conf = json.loads(srvPackData)
				for c in conf['server_packs']:
					if c['id'] == serverId:
						return c
				return None
	
	def getTotalFilesSize(self):
		max = 0
		with zipfile.ZipFile(self.archive, "r") as z:
			for zinfo in z.infolist():
				max += zinfo.file_size;
		return max

def MachineTypeToArchitectureSize(type):
	# List available from:
	#   http://www.microsoft.com/whdc/system/platform/firmware/PECOFF.mspx
	# or from Microsoft's PE COFF spec:
	#   http://msdn.microsoft.com/library/windows/hardware/gg463125
	machinetypes = {
		0x0000:  0, # IMAGE_FILE_MACHINE_UNKNOWN    The contents of this field are assumed to be applicable to any machine type
		0x014c: 32, # IMAGE_FILE_MACHINE_I386       Intel 386 or later processors and compatible processors
		0x0166: 32, # IMAGE_FILE_MACHINE_R4000      MIPS little endian
		0x0169: 32, # IMAGE_FILE_MACHINE_WCEMIPSV2  MIPS little-endian WCE v2
		0x01a2:  0, # IMAGE_FILE_MACHINE_SH3        Hitachi SH3
		0x01a3:  0, # IMAGE_FILE_MACHINE_SH3DSP     Hitachi SH3 DSP
		0x01a6:  0, # IMAGE_FILE_MACHINE_SH4        Hitachi SH4
		0x01a8:  0, # IMAGE_FILE_MACHINE_SH5        Hitachi SH5
		0x01c0: 32, # IMAGE_FILE_MACHINE_ARM        ARM little endian
		0x01c2: 32, # IMAGE_FILE_MACHINE_THUMB      ARM or Thumb ("interworking")
		0x01c4: 32, # IMAGE_FILE_MACHINE_ARMNT      ARMv7 (or higher) Thumb mode only
		0x01d3: 32, # IMAGE_FILE_MACHINE_AM33       Matsushita AM33
		0x01f0: 32, # IMAGE_FILE_MACHINE_POWERPC    Power PC little endian
		0x01f1: 32, # IMAGE_FILE_MACHINE_POWERPCFP  Power PC with floating point support
		0x0200: 64, # IMAGE_FILE_MACHINE_IA64       Intel Itanium processor family
		0x0266: 16, # IMAGE_FILE_MACHINE_MIPS16     MIPS16
		0x0366: 32, # IMAGE_FILE_MACHINE_MIPSFPU    MIPS with FPU
		0x0466: 16, # IMAGE_FILE_MACHINE_MIPSFPU16  MIPS16 with FPU
		0x0ebc:  0, # IMAGE_FILE_MACHINE_EBC        EFI byte code
		0x8664: 64, # IMAGE_FILE_MACHINE_AMD64      x64
		0x9041: 32, # IMAGE_FILE_MACHINE_M32R       Mitsubishi M32R little endian
		0xaa64: 64, # IMAGE_FILE_MACHINE_ARM64      ARMv8 in 64-bit mode
	}
	if type in machinetypes:
		return machinetypes[type]
	return 0

def archSize(exePath):
	with open(exePath, 'rb') as f:
		f.seek(0x3c)
		# offset is 32 bits signed int
		tmp = f.read(4)
		offset = struct.unpack('<i', tmp)[0]
		logger.debug("offset: %s", offset)
		f.seek(offset)
		# peHead is 32 bits unsigned signed int
		tmp = f.read(4)
		peHead = struct.unpack('<I', tmp)[0]
		logger.debug("peHead: %s", peHead)
		# "PE\0\0", little-endian
		if (peHead != 0x00004550):
			raise RuntimeError("Can't find PE header in {0}".format(exePath))
		# machineType is 16 bits unsigned signed short
		tmp = f.read(2)
		machineType = struct.unpack('<H', tmp)[0]
		return MachineTypeToArchitectureSize(machineType)

class game(progressreport):
	
	def __init__(self, gamedir, projectid):
		self.gamedir = gamedir
		self.projectid = projectid
		self.projectDir = os.path.join(gamedir, projectid)
	
	def loadConf(self):
		confPath = os.path.join(self.projectDir, "GameConfiguration.json")
		with open(confPath, 'r') as f:
			return json.load(f)
	
	def saveConf(self, conf):
		confPath = os.path.join(self.projectDir, "GameConfiguration.json")
		with open(confPath, 'w') as f:
			json.dump(conf, f, indent=3, sort_keys=True)
	
	def install(self, mcsPath, project, overwrite=False, cleanup=False):
		try:
			conf = self.loadConf()
		except:
			# if file does not exist or is invalid, assume empty conf.
			conf = {}
		# startup, dataPack = project.getstartup(startupid)
		for dataPack in project._datapacks():
			dataPackId = dataPack.get('id');
			dataPackDir = os.path.join(self.projectDir, dataPackId)
			self.installData(dataPack, dataPackDir, overwrite, cleanup)
		if not 'startups' in conf:
			conf['startups'] = {}
		for startup in project._startups():
			startupid = startup.get('id')
			fileHooks = startup.get('FileHooks')
			customProps = startup.get('CustomProperties')
			newStatup = {
				'id': startupid,
				'executable': startup['executable'],
				'dataDir': startup['dataPackId'],
				'workDir': startup['workDir'],
				'arguments': startup['arguments'],
				'FileHooks': fileHooks,
				'CustomProperties': customProps}
			conf['startups'][startupid] = newStatup
			self.saveConf(conf)
			exe = os.path.join(self.projectDir, startup['dataPackId'], startup['executable'])
			self.linkDlls(exe, mcsPath)
	
	def installData(self, datapack, datapackdir, overwrite, cleanup):
		datapackFiles = _getFilesFromDataPack(datapack)
		for root, dirs, files in os.walk(datapackdir):
			for f in files:
				destPath = os.path.join(root, f)
				aliasPath = os.path.relpath(destPath, datapackdir)
				srcPath = datapackFiles.pop(aliasPath, None)
				if srcPath and os.path.exists(srcPath):
					fileSize = os.path.getsize(srcPath)
					if overwrite or (os.path.getmtime(destPath) < os.path.getmtime(srcPath)) or (os.path.getsize(destPath) != fileSize):
						logger.debug("delete file %s", destPath)
						os.chmod(destPath, stat.S_IWRITE)
						os.remove(destPath)
						dir = os.path.dirname(destPath)
						if not os.path.isdir(dir):
							os.makedirs(dir)
						shutil.copy(srcPath, destPath)
					else:
						logger.debug("skip unchanged file %s", destPath)
					self.progressUpdate(fileSize)
				else:
					if cleanup:
						logger.debug("delete old file %s", destPath)
						os.chmod(destPath, stat.S_IWRITE)
						os.remove(destPath)
		for alias, file in datapackFiles.items():
			if not os.path.exists(file):
				raise RuntimeError("File {0} cannot be found or link cannot be resolved.".format(file))
			else:
				dest = os.path.join(datapackdir, alias)
				fileSize = os.path.getsize(file)
				dir = os.path.dirname(dest)
				if not os.path.isdir(dir):
					os.makedirs(dir)
				logger.debug("copy file %s to %s", file, dest)
				shutil.copy(file, dest)
				self.progressUpdate(fileSize)
	
	def linkDlls(self, exePath, mcsPath):
		exeDir = os.path.dirname(exePath)
		arch = archSize(exePath)
		if arch == 32:
			mcsDir = os.path.join(mcsPath, 'Shinra', 'x32', 'Release')
		elif arch == 64:
			mcsDir = os.path.join(mcsPath, 'Shinra', 'x64', 'Release')
		else:
			raise RuntimeError("Failed to get arch type for {0}".format(exePath))
		for linkName, dllName in MCS_DLL_MAPPING.items():
			linkTarget = os.path.join(mcsDir, dllName)
			linkPath = os.path.join(exeDir, linkName)
			checkDllLink(linkPath, linkTarget)
	
	def createCloudProperties(self, cloudPropertiesFilePath, startup, userid, gameport, videoport, mcsConf):
		storepath = mcsConf.get("StorePath")
		if not storepath.endswith("\\"):
			storepath += "\\"
		properties = {
			"Cloud": {
				"Mode": "local",
				"Debug": {
					"BreakOnStartup": False
				},
				"Performance": {
					"MaxFPS": 30
				},
				"Network": {
					"GamePort": gameport,
					"VideoPort": videoport
				},
				"Session": {
					"GameId": startup['id'],
					"UserId": userid,
					"TokenTimeoutSec": 120,
					"Token": "2OqFxPXSZlz9urM4DLU05wjND2WPtgruBW6ru3gmso4UwxnHb40ZMsuJ3LaqIQ8q"
				},
				"Audio": {
					"Enable": True
				},
				"Local": {
					"ShowWindow": mcsConf.get("ShowWindow"),
					"DisplayStatistics": mcsConf.get("DisplayStatistics"),
					"EncoderThreads": mcsConf.get("EncoderThreads"),
					"MaxRenderingFrames": mcsConf.get("MaxRenderingFrames"),
					"MaxEncodingFrames": mcsConf.get("MaxEncodingFrames")
				},
				"CloudCore": {
					"D3D9": "CloudCoreClient.dll",
					"D3D11": "CloudCoreClient.dll",
					"XInput": "CloudCoreClient.dll",
					"AppDataPath": storepath,
				},
				"FileHook": 
				{
					"TempFilteredPaths": [],
					"CloudFilteredPaths": [],
				}
			}
		}
		properties['Cloud']['FileHook'] = startup.get('FileHooks')
		customProperties = startup.get('CustomProperties')
		if customProperties:
			for property in customProperties:
				keys = property.split('.')
				x = properties['Cloud']
				for key in keys[:-1]:
					if not key in x:
						x[key] = {}
					x = x[key]
				x[keys[-1]] = customProperties[property]
		with open(cloudPropertiesFilePath, "w") as f:
			json.dump(properties, f, indent=3, sort_keys=True)
	
	def getstartup(self, id):
		try:
			conf = self.loadConf()
		except:
			raise RuntimeError("Failed to load configuration file. Make sure the game was properly installed.")
		if not 'startups' in conf:
			raise RuntimeError("Game configuration in {0} does not contain startups section".format(self.projectDir))
		startups = conf['startups']
		if not id in startups:
			raise RuntimeError("Game configuration in {0} does not contain startup {1}".format(self.projectDir, id))
		startup = startups.get(id)
		if not startup:
			raise RuntimeError("Game configuration in {0} contains empty configuration for startup {1}".format(self.projectDir, id))
		executable = startup.get('executable')
		if not executable:
			raise RuntimeError("Game configuration in {0} does not contain executable for startup {1}".format(self.projectDir, id))
		dataDir = startup.get('dataDir')
		if not dataDir:
			raise RuntimeError("Game configuration in {0} does not contain dataDir for startup {1}".format(self.projectDir, id))
		exePath = os.path.join(self.projectDir, dataDir, executable)
		if not os.path.isfile(exePath):
			raise RuntimeError("Game configuration in {0} has startup {1} with invalid executable: {2}".format(self.projectDir, startid, exePath))
		startup['exeFullPath'] = exePath
		if not 'workDir' in startup:
			raise RuntimeError("Game configuration in {0} does not contain workDir for startup {1}".format(self.projectDir, id))
		workDir = startup['workDir']
		if not workDir:
			startup['workDir'] = ""
		if not 'arguments' in startup:
			raise RuntimeError("Game configuration in {0} does not contain arguments for startup {1}".format(self.projectDir, id))
		return startup
	
	def setup(self, cloudPropertiesFileName, startup, startid, userid, gameport, videoport, mcsConf):
		logger.debug("Setup %s for user %s with gameport %s, videoport %s", startid, userid, gameport, videoport)
		exe = startup['exeFullPath']
		exedir = os.path.dirname(exe)
		cloudPropertiesFilePath = os.path.join(exedir, cloudPropertiesFileName)
		self.prepareUserFiles(userid, startid, mcsConf.get("StorePath"), mcsConf.get("ArchivePath"))
		self.createCloudProperties(cloudPropertiesFilePath, startup, userid, gameport, videoport, mcsConf)
		logger.debug("Created cloud properties at: %s", cloudPropertiesFilePath)
	
	def cleanup(self, cloudPropertiesFileName, startup, startid, userid, mcsConf):
		logger.debug("Cleanup %s for user %s", startid, userid)
		if cloudPropertiesFileName:
			exe = startup['exeFullPath']
			exedir = os.path.dirname(exe)
			cloudPropertiesFilePath = os.path.join(exedir, cloudPropertiesFileName)
			if os.path.exists(cloudPropertiesFilePath):
				os.remove(cloudPropertiesFilePath)
		self.saveUserFiles(userid, startid, mcsConf.get("StorePath"), mcsConf.get("ArchivePath"))
	
	def run(self, startid, userid, gameport, videoport, mcsConf):
		logger.debug("Starting %s for user %s with gameport %s, videoport %s", startid, userid, gameport, videoport)
		startup = self.getstartup(startid)
		exe = startup['exeFullPath']
		workDir = startup['workDir']
		cloudPropertiesFilePath = "CloudProperties_{0}_{1}.json".format(startup['id'], time.time())
		self.setup(cloudPropertiesFilePath, startup, startid, userid, gameport, videoport, mcsConf)
		env = os.environ.copy()
		env["FLARE_CONFIG"] = cloudPropertiesFilePath
		cwd = os.path.join(self.projectDir, startup['dataDir'], workDir)
		args = startup.get('arguments')
		if args:
			args = args.replace("{UserId}", userid)
		cmdline = "\"{0}\" {1}".format(exe, args)
		try:
			cmd = shlex.split(cmdline)
			logger.debug("Starting %s with cwd=%s, env=%s", cmd, cwd, env)
			with subprocess.Popen(cmd, cwd=cwd, env=env) as p:
				# expose the game process ID, so the GUI can kill the game if needed.
				# GUI should not kill the script itself.
				print("Game process id: {0}\n".format(p.pid))
				sys.stdout.flush()
				p.wait()
			logger.debug("Game ended")
			time.sleep(2)
		finally:
			self.cleanup(cloudPropertiesFilePath, startup, startid, userid, mcsConf)
	
	def prepareUserFiles(self, userid, gameid, storePath, archivePath):
		logger.debug("Restore user files %s %s", userid, gameid)
		tempDir = os.path.join(storePath, "UserFiles", userid, gameid, "Temp")
		cloudDir = os.path.join(storePath, "UserFiles", userid, gameid, "Cloud")
		archivePath = os.path.join(archivePath, userid, gameid + ".tar.gz")
		if not os.path.isdir(tempDir):
			os.makedirs(tempDir)
		if not os.path.isdir(cloudDir):
			os.makedirs(cloudDir)
		if os.path.isfile(archivePath):
			gameDir = os.path.dirname(cloudDir)
			logger.debug("Untar %s to %s", archivePath, gameDir)
			with tarfile.open(archivePath, 'r:gz') as t:
				t.extractall(gameDir)
	
	# clean both the directories which are not in the hashes file, and the directories which contain nothing
	def cleanUnusedDirectories(self, cloudDir):
		if os.path.isdir(cloudDir):
			hashesFile = os.path.join(cloudDir,'hashes.txt')
			logging.debug("Looking for {0} for unused directories...".format(hashesFile))
			hashList = []
			if os.path.isfile(hashesFile):
				file = open(hashesFile, 'r')
				for line in file:
					splitLine = line.split(' => ')
					hashList.append(splitLine[0])
				
				# check if every directory is in the hashes file. if not, delete it
				cloudDirList = os.listdir(cloudDir)
				for cDir in cloudDirList:
					cloudDirPath = os.path.join(cloudDir, cDir)
					if os.path.isdir(cloudDirPath):
						if not cDir in hashList:
							logging.debug("{0} is not in hash list => removing it.".format(cloudDirPath))
							remove_dir(cloudDirPath)
							
			# check the remaining directories and remove them if they contain nothing
			cloudDirList = os.listdir(cloudDir)
			for cDir in cloudDirList:
				cloudDirPath = os.path.join(cloudDir, cDir)
				if os.path.isdir(cloudDirPath) and not os.listdir(cloudDirPath):
					logging.debug("{0} contains nothing => removing it.".format(cloudDirPath))
					remove_dir(cloudDirPath)

	def saveUserFiles(self, userid, gameid, storePath, archivePath):
		logger.debug("Save user files %s %s", userid, gameid)
		tempDir = os.path.join(storePath, "UserFiles", userid, gameid, "Temp")
		cloudDir = os.path.join(storePath, "UserFiles", userid, gameid, "Cloud")
		archivePath = os.path.join(archivePath, userid, gameid + ".tar.gz")
		self.cleanUnusedDirectories(cloudDir)
		archivesDir = os.path.dirname(archivePath)
		if not os.path.isdir(archivesDir):
			os.makedirs(archivesDir)
		make_tarfile(archivePath, cloudDir)
		remove_dir(tempDir)
		remove_dir(cloudDir)

def add_gamesInstall_path_opt(parser, defConf):
	parser.add_argument('-games-install-dir', help="Directory where games will be installed for local execution.", default=conf['GamesInstallDir'])
	
def add_mcs_path_opt(parser, defConf):
	parser.add_argument('-mcs-path', help="Path where MCS is installed.", default=defConf['MCSPath'])

def add_overwrite_opt(parser, defConf):
	parser.add_argument('-overwrite', dest='overwrite', help="Force overwrite of all files on install.", action='store_true')
	parser.add_argument('-no-overwrite', dest='overwrite', help="Only files with a older modification date, or a different size than the source will be overwritten on install.",
	action='store_false')
	parser.set_defaults(overwrite=defConf['OverwriteGameInstall'])

def add_cleanup_opt(parser, defConf):
	parser.add_argument('-cleanup', dest='cleanup', help="Force deletion of files in the installation directory that are not part of the datapack.", action='store_true')
	parser.add_argument('-no-cleanup', dest='cleanup', help="Leave files in the installation directory that are not part of the datapack.", action='store_false')
	parser.set_defaults(cleanup=defConf['CleanupOnInstall'])

def add_store_path_opt(parser, defConf):
	parser.add_argument('-store-path', help="Working directory where user data will be stored during game execution.", default=defConf['StorePath'])

def add_archive_path_opt(parser, defConf):
	parser.add_argument('-archive-path', help="Directory where user data will be archived between two game executions.", default=defConf['ArchivePath'])

def add_show_window_opt(parser, defConf):
	parser.add_argument('-show-window', dest='show_window', help="Show renderer window.", action='store_true')
	parser.add_argument('-no-show-window', dest='show_window', help="Hide renderer window.", action='store_false')
	parser.set_defaults(show_window=defConf['ShowWindow'])

def add_display_statistics_opt(parser, defConf):
	parser.add_argument('-display-statistics', dest='display_statistics', help="Display statistics in renderer window. Ineffective if -no-show-window.", action='store_true')
	parser.add_argument('-no-display-statistics', dest='display_statistics', help="Hide statistics in renderer window. Ineffective if -no-show-window.", action='store_false')
	parser.set_defaults(display_statistics=defConf['DisplayStatistics'])

def add_encoder_threads_opt(parser, defConf):
	parser.add_argument('-encoder-threads', help="The number of threads dedicated to encoding the video stream.", type=int, default=defConf['EncoderThreads'])

def add_max_encoding_frames_opt(paser, defConf):
	paser.add_argument('-max-encoding-frames', help="The maximum number of queued encoding frame requested before the game start to block.", type=int, default=defConf['MaxEncodingFrames'])

def add_max_rendering_frames_opt(parser, defConf):
	parser.add_argument('-max-rendering-frames', help="The maximum number of queued rendering frame requested before the game start to block.", type=int, default=defConf['MaxRenderingFrames'])

if __name__ == "__main__":
	conf = getMCSSettings()
	
	parser = argparse.ArgumentParser(description='Shinra MCS management script.')
	subparsers = parser.add_subparsers(help='command help.', dest='command')
	parser_pkg = subparsers.add_parser('package', help='Create a Shinra package from a project.')
	parser_pkg.add_argument('project', help="Shinra project to use.")
	parser_pkg.add_argument('archive', help="Shinra package to create.")
	
	parser_instgame = subparsers.add_parser('install', help='Install a game from a project.')
	parser_instgame.add_argument('project', help="Shinra project to install.")
	add_gamesInstall_path_opt(parser_instgame, conf)
	add_mcs_path_opt(parser_instgame, conf)
	add_overwrite_opt(parser_instgame, conf)
	add_cleanup_opt(parser_instgame, conf)
	
	parser_instarch = subparsers.add_parser('project', help='Build a project from a package.')
	parser_instarch.add_argument('archive', help="Shinra package to use as source.")
	parser_instarch.add_argument('dir', help="Directory where to store project data.")
	parser_instarch.add_argument('project', help="Name of Shinra project file to create.")
	
	parser_rungame = subparsers.add_parser('run', help='Run an installed game.')
	parser_rungame.add_argument('projectid', help="Project id of the game to run.")
	parser_rungame.add_argument('gameid', help="Startup id to use to run the game.")
	parser_rungame.add_argument('userid', help="User id to use to run the game.")
	parser_rungame.add_argument('gameport', help="Port to use for game communications.", type=int)
	parser_rungame.add_argument('videoport', help="Port to use for video communications.", type=int)
	add_gamesInstall_path_opt(parser_rungame, conf)
	add_store_path_opt(parser_rungame, conf)
	add_archive_path_opt(parser_rungame, conf)
	add_show_window_opt(parser_rungame, conf)
	add_display_statistics_opt(parser_rungame, conf)
	add_encoder_threads_opt(parser_rungame, conf)
	add_max_encoding_frames_opt(parser_rungame, conf)
	add_max_rendering_frames_opt(parser_rungame, conf)
	
	parser_setupgame = subparsers.add_parser('setup', help='Setup a game instance for manual execution. Restore the user data from the archive and create proper settings files.')
	parser_setupgame.add_argument('projectid', help="Project id of the game to run.")
	parser_setupgame.add_argument('gameid', help="Startup id to use to run the game.")
	parser_setupgame.add_argument('userid', help="User id to use to run the game.")
	parser_setupgame.add_argument('gameport', help="Port to use for game communications.", type=int)
	parser_setupgame.add_argument('videoport', help="Port to use for video communications.", type=int)
	parser_setupgame.add_argument('-cloud-properties', help="File name to use for creating the cloud property file.")
	add_gamesInstall_path_opt(parser_setupgame, conf)
	add_store_path_opt(parser_setupgame, conf)
	add_archive_path_opt(parser_setupgame, conf)
	add_show_window_opt(parser_setupgame, conf)
	add_display_statistics_opt(parser_setupgame, conf)
	add_encoder_threads_opt(parser_setupgame, conf)
	add_max_encoding_frames_opt(parser_setupgame, conf)
	add_max_rendering_frames_opt(parser_setupgame, conf)
	
	parser_cleanupgame = subparsers.add_parser('cleanup', help='Cleanup a game instance after a game execution. Archive user data and cleanup temporary files.')
	parser_cleanupgame.add_argument('projectid', help="Project id of the game to run.")
	parser_cleanupgame.add_argument('userid', help="User id to use to run the game.")
	parser_cleanupgame.add_argument('gameid', help="Startup id to use to run the game.")
	parser_cleanupgame.add_argument('-cloud-properties', help="File name of the cloud property file to cleanup.")
	add_gamesInstall_path_opt(parser_cleanupgame, conf)
	add_store_path_opt(parser_cleanupgame, conf)
	add_archive_path_opt(parser_cleanupgame, conf)
	
	parser_setconfig = subparsers.add_parser('configure', help='Set default MCS settings values for current user. Configuration file for current user is {0}.'.format(configurationFile))
	add_store_path_opt(parser_setconfig, conf)
	add_archive_path_opt(parser_setconfig, conf)
	#parser_setconfig.add_argument('-games-install-dir', help="Directory where games will be installed for local execution.", default=conf['GamesInstallDir'])
	add_show_window_opt(parser_setconfig, conf)
	add_display_statistics_opt(parser_setconfig, conf)
	add_encoder_threads_opt(parser_setconfig, conf)
	add_max_encoding_frames_opt(parser_setconfig, conf)
	add_max_rendering_frames_opt(parser_setconfig, conf)
	add_mcs_path_opt(parser_setconfig, conf)
	add_overwrite_opt(parser_setconfig, conf)
	add_cleanup_opt(parser_setconfig, conf)

	parser.add_argument('--log-level', choices=['DEBUG', 'INFO', 'WARN', 'ERROR'])
	parser.add_argument('--log-file')
	
	args = parser.parse_args()
	
	logLevel = logging.DEBUG
	if args.log_level:
		logLevel = getattr(logging, args.log_level, None)
	if args.log_file:
		ch = logging.FileHandler(args.log_file)
	else:
		ch = logging.StreamHandler(sys.stdout)
	rootLogger = logging.getLogger()
	rootLogger.setLevel(logLevel)
	rootLogger.addHandler(ch)
	
	if args.command == 'package':
		p = project(args.project)
		a = package(args.archive)
		totalsize = p.getTotalFilesSize()
		a.progressInit(totalsize)
		a.compress(p)
	elif args.command == 'install':
		p = project(args.project)
		g = game(args.games_install_dir, p.projectId())
		totalsize = p.getTotalFilesSize()
		g.progressInit(totalsize)
		overwrite = False
		if args.overwrite:
			overwrite = args.overwrite
		cleanup = False
		if args.cleanup:
			cleanup = args.cleanup
		g.install(args.mcs_path, p, overwrite, cleanup)
	elif args.command == 'run':
		logger.debug("args.games_install_dir=%s, args.projectid=%s", args.games_install_dir, args.projectid)
		g = game(args.games_install_dir, args.projectid)
		conf["StorePath"] = args.store_path
		conf["ArchivePath"] = args.archive_path
		conf['DisplayStatistics'] = args.display_statistics
		conf['EncoderThreads'] = args.encoder_threads
		conf['MaxEncodingFrames'] = args.max_encoding_frames
		conf['MaxRenderingFrames'] = args.max_rendering_frames
		conf['ShowWindow'] = args.show_window
		validateMCSSettings(conf)
		logger.debug("args.gameid=%s, args.userid=%s, args.gameport=%s, args.videoport=%s, conf=%s", args.gameid, args.userid, args.gameport, args.videoport, conf)
		g.run(args.gameid, args.userid, args.gameport, args.videoport, conf)
	elif args.command == 'setup':
		logger.debug("cmd args: %s", args)
		g = game(args.games_install_dir, args.projectid)
		startup = g.getstartup(args.gameid)
		cloudProperties = "CloudProperties.json"
		if args.cloud_properties:
			cloudProperties = args.cloud_properties
		logger.debug("conf before = %s", conf)
		conf["StorePath"] = args.store_path
		conf["ArchivePath"] = args.archive_path
		logger.debug("args.display_statistics %s", args.display_statistics)
		conf['DisplayStatistics'] = args.display_statistics
		conf['EncoderThreads'] = args.encoder_threads
		conf['MaxEncodingFrames'] = args.max_encoding_frames
		conf['MaxRenderingFrames'] = args.max_rendering_frames
		conf['ShowWindow'] = args.show_window
		logger.debug("conf after = %s", conf)
		validateMCSSettings(conf)
		g.setup(cloudProperties, startup, args.gameid, args.userid, args.gameport, args.videoport, conf)
	elif args.command == 'cleanup':
		g = game(args.games_install_dir, args.projectid)
		startup = g.getstartup(args.gameid)
		cloudProperties = None
		if args.cloud_properties:
			cloudProperties = args.cloud_properties
		conf["StorePath"] = args.store_path
		conf["ArchivePath"] = args.archive_path
		validateMCSSettings(conf)
		g.cleanup(cloudProperties, startup, args.gameid, args.userid, conf)
	elif args.command == 'configure':
		conf['StorePath'] = args.store_path
		conf['ArchivePath'] = args.archive_path
		conf['GamesInstallDir'] = args.games_install_dir
		conf['DisplayStatistics'] = args.display_statistics
		conf['EncoderThreads'] = args.encoder_threads
		conf['MaxEncodingFrames'] = args.max_encoding_frames
		conf['MaxRenderingFrames'] = args.max_rendering_frames
		conf['ShowWindow'] = args.show_window
		conf['MCSPath'] = args.mcs_path
		conf['OverwriteGameInstall'] = args.overwrite_game_install
		conf['CleanupOnInstall'] = args.cleanup_on_install
		validateMCSSettings(conf)
		setMCSSettings(conf)
	elif args.command == 'project':
		a = package(args.archive)
		totalsize = 2 * a.getTotalFilesSize()
		a.progressInit(totalsize)
		a.validate()
		a.uncompress(args.dir)
		a.createproject(args.project, args.dir)



