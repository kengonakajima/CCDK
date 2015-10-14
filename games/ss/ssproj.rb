#
# ssproj.rb
#
#
#

require "redis"  # gem install redis
require "json"

USAGE=<<EOS

ssproj.rb: Validate/modify Space Sweeper projects stored in Redis and filesystem.

Usage: ruby ssproj.rb DATADIR PROJECTID [OPTIONS]

With no option, it check current status of the project.

OPTIONS:
  --delete : Delete the project

Example:
  ruby ssproj.rb /var/ss/datadir 37 --delete

EOS

if ARGV.size < 2 then
  print USAGE
  exit 1
end

$datadir = ARGV[0]
$projid = ARGV[1].to_i
$to_delete = false

if ARGV[2] == "--delete" then
  $to_delete = true
end

print "Datadir:'#{$datadir}' ProjectId:#{$projid} to_delete:#{$to_delete}\n"

redis = Redis.new

# hash tables
pj_by_owner_h_key = "pj_by_owner_h" 
pj_info_h_key = "pj_info_h" 
project_status_h_key = "project_status_h" 
ms_progress_z_key = "ms_progress_z" # 'z' means set
all_clear_z_key = "all_clear_z"

# project list
pj_by_owner_h = redis.hgetall( pj_by_owner_h_key )
pj_by_owner_h.keys.each do |owner|
  pjlist = JSON.parse(pj_by_owner_h[owner])
  # print owner," has ", pjlist,"\n"  
  if pjlist.index($projid) then
    print "Project #{$projid} found in #{owner} : #{pjlist}\n"
    if $to_delete then
      print "delete and save..\n"
      pjlist.delete($projid)
      res = redis.hset( pj_by_owner_h_key, owner, pjlist.to_s )
      print "redis result: #{res}\n"
    end
  end
end

# project info
pjinfo = redis.hget( pj_info_h_key, $projid )
if pjinfo then
  print "Project info data size:#{pjinfo.size}\n"
  if $to_delete then
    print "deleting project info... "
    res = redis.hdel( pj_info_h_key, $projid )
    print "redis result: #{res}\n"
  end
else
  print "Project info not found in #{pj_info_h_key}\n"
end

# project status
status = redis.hget( project_status_h_key, $projid )
if status then 
  print "Project status found: #{status}\n"
else
  print "Project status not found\n"
end

# milestone progress log
mslogs = redis.zrange( ms_progress_z_key, 0, -1, :with_scores => true )

print "milestone progress:\n"
mslogs.each do |pair|
  print "pair:", pair, "\n"
  if pair[1].to_i == $projid then
    if $to_delete then
      print "deleting progress:", pair, "\n"
      redis.zrem( ms_progress_z_key, pair[0] )
    end
  end
end

# all clear log
allclear = redis.zrange( all_clear_z_key, 0, -1, :with_scores => false )
print "all clear:", allclear,"\n"
if $to_delete then
  print "all clear: project found. delete #{$projid}!\n"
  redis.zrem( all_clear_z_key, $projid )
end

def tryDeleteDataFiles(pat)
  out = `find #{$datadir} -name '#{pat}'`.split
  out.each do |fn|
    if $to_delete then
      print "Deleting file '#{fn}'...\n"
      File.delete(fn)
    else
      print "File '#{fn}' found!\n"
    end
  end
  if out.size == 0 then
    print "File '#{pat}' is not found in #{$datadir} .\n"
  end
end

# field data file
# Don't emulate backend server's hash_pjw function for directory names. Use 'find' but it is a bit slow. (scans 64K directories

tryDeleteDataFiles( "field_data_#{$projid}_2048_2048_48" )
tryDeleteDataFiles( "powersystem_#{$projid}_part_*" )
tryDeleteDataFiles( "deposit_#{$projid}_part_*" )
tryDeleteDataFiles( "research_#{$projid}_part_*" )
tryDeleteDataFiles( "reveal_#{$projid}_part_*" )
tryDeleteDataFiles( "envs_#{$projid}_part_*" )
tryDeleteDataFiles( "_image_#{$projid}" )
tryDeleteDataFiles( "_counter_cat100_id#{$projid}" )

