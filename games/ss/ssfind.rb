# ssfind: find project IDs from datadir

# depending on UNIX find command.



if ARGV.size != 1 then
  STDERR.print "Usage: ruby ssfind.rb DATADIR\n"
  exit 1
end

datadir = ARGV[0]

STDERR.print "datadir: #{datadir}\n"

allfiles = `find #{datadir} | grep field_data`.split

projects = {}

allfiles.each do |fn|
  bn = File.basename(fn)
#  print bn, "\n"
  bn =~ /field_data_([0-9]+)_([0-9]+)_([0-9]+)_([0-9]+)_([0-9]+)/
  pjid = $1
  if projects[pjid] == nil then
    projects[pjid] = 1
  else
    projects[pjid] += 1
  end
end

projects.keys.each do |pjid|
  print "Project #{pjid} : #{projects[pjid]} files found\n"
end
