# Generated by gen.rb CommunityEngine Inc. 2000-2005
require "vce.so"


SSPROTO_USERID_SIZE_MAX = 32
SSPROTO_OK = 0
SSPROTO_E_FILE_NAME_INVALID = -1
SSPROTO_E_FILE_NAME_TOO_LONG = -2
SSPROTO_E_FILE_ACCESS = -3
SSPROTO_E_KVS_COMMAND = -10
SSPROTO_E_KVS_ARRAY_NOT_IMPLEMENTED = -11
SSPROTO_E_CHANNEL_FULL = -30
SSPROTO_E_CHANNEL_JOIN_REQUIRED = -31
SSPROTO_E_CANT_LOCK = -40
SSPROTO_E_CANT_UNLOCK = -41
SSPROTO_E_IMAGE_STORE_CANNOT_ALLOCATE = -42
SSPROTO_E_IMAGE_INVALID_SIZE = -43
SSPROTO_E_IMAGE_NOT_FOUND = -44
SSPROTO_E_IMAGE_CANNOT_CONVERT = -45
SSPROTO_E_PROJECT_NOT_SHARED = -46
SSPROTO_FILE_PATH_MAX = 64
SSPROTO_FILE_SIZE_MAX = 131072
SSPROTO_FILE_SIZE_ABS_MAX = 1048576
SSPROTO_FILE_EXIST = 1
SSPROTO_FILE_NOT_EXIST = 0
SSPROTO_IMAGE_WIDTH_MAX = 2048
SSPROTO_IMAGE_HEIGHT_MAX = 2048
SSPROTO_IMAGE_PART_WIDTH_MAX = 32
SSPROTO_IMAGE_PART_HEIGHT_MAX = 32
SSPROTO_PNG_SIZE_MAX = 1048576
SSPROTO_RAW_IMAGE_SIZE_MAX = 1048576
SSPROTO_GENERATE_ID_NUM_MAX = 100
SSPROTO_KVS_COMMAND_MAX = 1024
SSPROTO_KVS_ELEMENT_MAX = 1024
SSPROTO_KVS_ARRAYLEN_MAX = 1024
SSPROTO_KVS_VALUE_STRING = 1
SSPROTO_KVS_VALUE_ARRAY = 2
SSPROTO_KVS_VALUE_INTEGER = 3
SSPROTO_KVS_VALUE_NIL = 4
SSPROTO_KVS_VALUE_ERROR = 6
SSPROTO_KVS_KEY_MAX = 128
SSPROTO_KVS_FIELD_MAX = 128
SSPROTO_KVS_BIN_MAX = 262144
SSPROTO_SHARE_MAX = 128
SSPROTO_SEARCH_MAX = 20
SSPROTO_PROJECT_IS_NOT_SHARED = 0
SSPROTO_PROJECT_IS_SHARED = 1
SSPROTO_PROJECT_IS_PUBLISHED = 2
SSPROTO_PRESENCE_PER_PROJECT_MAX = 1024
SSPROTO_PACKET_SIZE_MAX = 65536
["$ssproto_ping_recv_counter = 0\n", "$ssproto_conn_serial_recv_counter = 0\n", "$ssproto_clean_all_recv_counter = 0\n", "$ssproto_put_file_recv_counter = 0\n", "$ssproto_get_file_recv_counter = 0\n", "$ssproto_check_file_recv_counter = 0\n", "$ssproto_ensure_image_recv_counter = 0\n", "$ssproto_update_image_part_recv_counter = 0\n", "$ssproto_get_image_png_recv_counter = 0\n", "$ssproto_get_image_raw_recv_counter = 0\n", "$ssproto_generate_id_32_recv_counter = 0\n", "$ssproto_kvs_command_str_recv_counter = 0\n", "$ssproto_kvs_command_str_oneway_recv_counter = 0\n", "$ssproto_kvs_push_to_list_recv_counter = 0\n", "$ssproto_kvs_get_list_range_recv_counter = 0\n", "$ssproto_kvs_append_string_array_recv_counter = 0\n", "$ssproto_kvs_get_string_array_recv_counter = 0\n", "$ssproto_kvs_save_bin_recv_counter = 0\n", "$ssproto_kvs_load_bin_recv_counter = 0\n", "$ssproto_counter_add_recv_counter = 0\n", "$ssproto_counter_get_recv_counter = 0\n", "$ssproto_share_project_recv_counter = 0\n", "$ssproto_publish_project_recv_counter = 0\n", "$ssproto_search_shared_projects_recv_counter = 0\n", "$ssproto_search_published_projects_recv_counter = 0\n", "$ssproto_project_is_joinable_recv_counter = 0\n", "$ssproto_unshare_project_recv_counter = 0\n", "$ssproto_unpublish_project_recv_counter = 0\n", "$ssproto_is_published_project_recv_counter = 0\n", "$ssproto_is_shared_project_recv_counter = 0\n", "$ssproto_update_presence_recv_counter = 0\n", "$ssproto_delete_presence_recv_counter = 0\n", "$ssproto_list_presence_recv_counter = 0\n", "$ssproto_count_presence_recv_counter = 0\n", "$ssproto_lock_grid_recv_counter = 0\n", "$ssproto_unlock_grid_recv_counter = 0\n", "$ssproto_lock_keep_grid_recv_counter = 0\n", "$ssproto_lock_project_recv_counter = 0\n", "$ssproto_unlock_project_recv_counter = 0\n", "$ssproto_lock_keep_project_recv_counter = 0\n", "$ssproto_broadcast_recv_counter = 0\n", "$ssproto_channelcast_recv_counter = 0\n", "$ssproto_join_channel_recv_counter = 0\n", "$ssproto_leave_channel_recv_counter = 0\n", "$ssproto_nearcast_recv_counter = 0\n", "$ssproto_update_nearcast_position_recv_counter = 0\n", "$ssproto_get_channel_member_count_recv_counter = 0\n", "$ssproto_pong_send_counter = 0\n", "$ssproto_version_notify_send_counter = 0\n", "$ssproto_conn_serial_result_send_counter = 0\n", "$ssproto_clean_all_result_send_counter = 0\n", "$ssproto_put_file_result_send_counter = 0\n", "$ssproto_get_file_result_send_counter = 0\n", "$ssproto_check_file_result_send_counter = 0\n", "$ssproto_ensure_image_result_send_counter = 0\n", "$ssproto_update_image_part_result_send_counter = 0\n", "$ssproto_get_image_png_result_send_counter = 0\n", "$ssproto_get_image_raw_result_send_counter = 0\n", "$ssproto_generate_id_32_result_send_counter = 0\n", "$ssproto_kvs_command_str_result_send_counter = 0\n", "$ssproto_kvs_push_to_list_result_send_counter = 0\n", "$ssproto_kvs_get_list_range_result_send_counter = 0\n", "$ssproto_kvs_append_string_array_result_send_counter = 0\n", "$ssproto_kvs_get_string_array_result_send_counter = 0\n", "$ssproto_kvs_save_bin_result_send_counter = 0\n", "$ssproto_kvs_load_bin_result_send_counter = 0\n", "$ssproto_counter_get_result_send_counter = 0\n", "$ssproto_share_project_result_send_counter = 0\n", "$ssproto_publish_project_result_send_counter = 0\n", "$ssproto_search_shared_projects_result_send_counter = 0\n", "$ssproto_search_published_projects_result_send_counter = 0\n", "$ssproto_project_is_joinable_result_send_counter = 0\n", "$ssproto_is_published_project_result_send_counter = 0\n", "$ssproto_is_shared_project_result_send_counter = 0\n", "$ssproto_list_presence_result_send_counter = 0\n", "$ssproto_count_presence_result_send_counter = 0\n", "$ssproto_lock_grid_result_send_counter = 0\n", "$ssproto_unlock_grid_result_send_counter = 0\n", "$ssproto_lock_project_result_send_counter = 0\n", "$ssproto_unlock_project_result_send_counter = 0\n", "$ssproto_broadcast_notify_send_counter = 0\n", "$ssproto_channelcast_notify_send_counter = 0\n", "$ssproto_join_channel_result_send_counter = 0\n", "$ssproto_leave_channel_result_send_counter = 0\n", "$ssproto_nearcast_notify_send_counter = 0\n", "$ssproto_get_channel_member_count_result_send_counter = 0\n"]
$ssproto_recvdata = []


$ssproto_sv_proc = Proc.new {|_c,_data|
  ret = 0
  break if( _data.length < 2 )
  _command = _data.pop_nb_i2!
  case _command
["  when 1\n      t_usec = _data.pop_nb_i8!()\n      cmd = _data.pop_nb_i4!()\n    $ssproto_ping_recv_counter += 1\n    ret = ssproto_ping_recv(_c, t_usec,cmd )\n", "  when 10\n    ret = ssproto_conn_serial_recv(_c)\n", "  when 12\n    ret = ssproto_clean_all_recv(_c)\n", "  when 20\n      query_id = _data.pop_nb_i4!()\n      filename = _data.pop_nb_stra!(64)\n      data = _data.pop_nb_ia1!(131072)\n    $ssproto_put_file_recv_counter += 1\n    ret = ssproto_put_file_recv(_c, query_id,filename,data )\n", "  when 30\n      query_id = _data.pop_nb_i4!()\n      filename = _data.pop_nb_stra!(64)\n    $ssproto_get_file_recv_counter += 1\n    ret = ssproto_get_file_recv(_c, query_id,filename )\n", "  when 40\n      query_id = _data.pop_nb_i4!()\n      filename = _data.pop_nb_stra!(64)\n    $ssproto_check_file_recv_counter += 1\n    ret = ssproto_check_file_recv(_c, query_id,filename )\n", "  when 50\n      query_id = _data.pop_nb_i4!()\n      image_id = _data.pop_nb_i4!()\n      w = _data.pop_nb_i4!()\n      h = _data.pop_nb_i4!()\n    $ssproto_ensure_image_recv_counter += 1\n    ret = ssproto_ensure_image_recv(_c, query_id,image_id,w,h )\n", "  when 52\n      query_id = _data.pop_nb_i4!()\n      image_id = _data.pop_nb_i4!()\n      x0 = _data.pop_nb_i4!()\n      y0 = _data.pop_nb_i4!()\n      w = _data.pop_nb_i4!()\n      h = _data.pop_nb_i4!()\n      r = _data.pop_nb_ia1!(1024)\n      g = _data.pop_nb_ia1!(1024)\n      b = _data.pop_nb_ia1!(1024)\n    $ssproto_update_image_part_recv_counter += 1\n    ret = ssproto_update_image_part_recv(_c, query_id,image_id,x0,y0,w,h,r,g,b )\n", "  when 54\n      query_id = _data.pop_nb_i4!()\n      image_id = _data.pop_nb_i4!()\n    $ssproto_get_image_png_recv_counter += 1\n    ret = ssproto_get_image_png_recv(_c, query_id,image_id )\n", "  when 56\n      query_id = _data.pop_nb_i4!()\n      image_id = _data.pop_nb_i4!()\n      x0 = _data.pop_nb_i4!()\n      y0 = _data.pop_nb_i4!()\n      w = _data.pop_nb_i4!()\n      h = _data.pop_nb_i4!()\n    $ssproto_get_image_raw_recv_counter += 1\n    ret = ssproto_get_image_raw_recv(_c, query_id,image_id,x0,y0,w,h )\n", "  when 100\n      query_id = _data.pop_nb_i4!()\n      num = _data.pop_nb_i4!()\n    $ssproto_generate_id_32_recv_counter += 1\n    ret = ssproto_generate_id_32_recv(_c, query_id,num )\n", "  when 120\n      query_id = _data.pop_nb_i4!()\n      command = _data.pop_nb_stra!(1024)\n    $ssproto_kvs_command_str_recv_counter += 1\n    ret = ssproto_kvs_command_str_recv(_c, query_id,command )\n", "  when 122\n      command = _data.pop_nb_stra!(1024)\n    $ssproto_kvs_command_str_oneway_recv_counter += 1\n    ret = ssproto_kvs_command_str_oneway_recv(_c, command )\n", "  when 130\n      query_id = _data.pop_nb_i4!()\n      key = _data.pop_nb_stra!(128)\n      s = _data.pop_nb_stra!(262144)\n      trim = _data.pop_nb_i4!()\n    $ssproto_kvs_push_to_list_recv_counter += 1\n    ret = ssproto_kvs_push_to_list_recv(_c, query_id,key,s,trim )\n", "  when 132\n      query_id = _data.pop_nb_i4!()\n      key = _data.pop_nb_stra!(128)\n      start_ind = _data.pop_nb_i4!()\n      end_ind = _data.pop_nb_i4!()\n    $ssproto_kvs_get_list_range_recv_counter += 1\n    ret = ssproto_kvs_get_list_range_recv(_c, query_id,key,start_ind,end_ind )\n", "  when 134\n      query_id = _data.pop_nb_i4!()\n      key = _data.pop_nb_stra!(128)\n      field = _data.pop_nb_stra!(128)\n      s = _data.pop_nb_stra!(1024)\n      trim = _data.pop_nb_i4!()\n    $ssproto_kvs_append_string_array_recv_counter += 1\n    ret = ssproto_kvs_append_string_array_recv(_c, query_id,key,field,s,trim )\n", "  when 136\n      query_id = _data.pop_nb_i4!()\n      key = _data.pop_nb_stra!(128)\n      field = _data.pop_nb_stra!(128)\n    $ssproto_kvs_get_string_array_recv_counter += 1\n    ret = ssproto_kvs_get_string_array_recv(_c, query_id,key,field )\n", "  when 140\n      query_id = _data.pop_nb_i4!()\n      key = _data.pop_nb_stra!(128)\n      field = _data.pop_nb_stra!(128)\n      data = _data.pop_nb_ia1!(262144)\n    $ssproto_kvs_save_bin_recv_counter += 1\n    ret = ssproto_kvs_save_bin_recv(_c, query_id,key,field,data )\n", "  when 150\n      query_id = _data.pop_nb_i4!()\n      key = _data.pop_nb_stra!(128)\n      field = _data.pop_nb_stra!(128)\n    $ssproto_kvs_load_bin_recv_counter += 1\n    ret = ssproto_kvs_load_bin_recv(_c, query_id,key,field )\n", "  when 160\n      counter_category = _data.pop_nb_i4!()\n      counter_id = _data.pop_nb_i4!()\n      addvalue = _data.pop_nb_i4!()\n    $ssproto_counter_add_recv_counter += 1\n    ret = ssproto_counter_add_recv(_c, counter_category,counter_id,addvalue )\n", "  when 161\n      counter_category = _data.pop_nb_i4!()\n      counter_id = _data.pop_nb_i4!()\n    $ssproto_counter_get_recv_counter += 1\n    ret = ssproto_counter_get_recv(_c, counter_category,counter_id )\n", "  when 180\n      user_id = _data.pop_nb_i4!()\n      project_id = _data.pop_nb_i4!()\n      with = _data.pop_nb_ia4!(128)\n    $ssproto_share_project_recv_counter += 1\n    ret = ssproto_share_project_recv(_c, user_id,project_id,with )\n", "  when 182\n      user_id = _data.pop_nb_i4!()\n      project_id = _data.pop_nb_i4!()\n    $ssproto_publish_project_recv_counter += 1\n    ret = ssproto_publish_project_recv(_c, user_id,project_id )\n", "  when 186\n      user_id = _data.pop_nb_i4!()\n    $ssproto_search_shared_projects_recv_counter += 1\n    ret = ssproto_search_shared_projects_recv(_c, user_id )\n", "  when 188\n    ret = ssproto_search_published_projects_recv(_c)\n", "  when 190\n      project_id = _data.pop_nb_i4!()\n      user_id = _data.pop_nb_i4!()\n    $ssproto_project_is_joinable_recv_counter += 1\n    ret = ssproto_project_is_joinable_recv(_c, project_id,user_id )\n", "  when 192\n      project_id = _data.pop_nb_i4!()\n    $ssproto_unshare_project_recv_counter += 1\n    ret = ssproto_unshare_project_recv(_c, project_id )\n", "  when 194\n      project_id = _data.pop_nb_i4!()\n    $ssproto_unpublish_project_recv_counter += 1\n    ret = ssproto_unpublish_project_recv(_c, project_id )\n", "  when 196\n      project_id = _data.pop_nb_i4!()\n    $ssproto_is_published_project_recv_counter += 1\n    ret = ssproto_is_published_project_recv(_c, project_id )\n", "  when 198\n      project_id = _data.pop_nb_i4!()\n      owner_uid = _data.pop_nb_i4!()\n    $ssproto_is_shared_project_recv_counter += 1\n    ret = ssproto_is_shared_project_recv(_c, project_id,owner_uid )\n", "  when 200\n      project_id = _data.pop_nb_i4!()\n      user_id = _data.pop_nb_i4!()\n    $ssproto_update_presence_recv_counter += 1\n    ret = ssproto_update_presence_recv(_c, project_id,user_id )\n", "  when 201\n      project_id = _data.pop_nb_i4!()\n      user_id = _data.pop_nb_i4!()\n    $ssproto_delete_presence_recv_counter += 1\n    ret = ssproto_delete_presence_recv(_c, project_id,user_id )\n", "  when 204\n      project_id = _data.pop_nb_i4!()\n    $ssproto_list_presence_recv_counter += 1\n    ret = ssproto_list_presence_recv(_c, project_id )\n", "  when 206\n      project_id = _data.pop_nb_i4!()\n    $ssproto_count_presence_recv_counter += 1\n    ret = ssproto_count_presence_recv(_c, project_id )\n", "  when 210\n      grid_id = _data.pop_nb_i4!()\n      x = _data.pop_nb_i4!()\n      y = _data.pop_nb_i4!()\n    $ssproto_lock_grid_recv_counter += 1\n    ret = ssproto_lock_grid_recv(_c, grid_id,x,y )\n", "  when 212\n      grid_id = _data.pop_nb_i4!()\n      x = _data.pop_nb_i4!()\n      y = _data.pop_nb_i4!()\n    $ssproto_unlock_grid_recv_counter += 1\n    ret = ssproto_unlock_grid_recv(_c, grid_id,x,y )\n", "  when 214\n      grid_id = _data.pop_nb_i4!()\n      x = _data.pop_nb_i4!()\n      y = _data.pop_nb_i4!()\n    $ssproto_lock_keep_grid_recv_counter += 1\n    ret = ssproto_lock_keep_grid_recv(_c, grid_id,x,y )\n", "  when 216\n      project_id = _data.pop_nb_i4!()\n      category = _data.pop_nb_i4!()\n    $ssproto_lock_project_recv_counter += 1\n    ret = ssproto_lock_project_recv(_c, project_id,category )\n", "  when 218\n      project_id = _data.pop_nb_i4!()\n      category = _data.pop_nb_i4!()\n    $ssproto_unlock_project_recv_counter += 1\n    ret = ssproto_unlock_project_recv(_c, project_id,category )\n", "  when 220\n      project_id = _data.pop_nb_i4!()\n      category = _data.pop_nb_i4!()\n    $ssproto_lock_keep_project_recv_counter += 1\n    ret = ssproto_lock_keep_project_recv(_c, project_id,category )\n", "  when 230\n      type_id = _data.pop_nb_i4!()\n      data = _data.pop_nb_ia1!(65536)\n    $ssproto_broadcast_recv_counter += 1\n    ret = ssproto_broadcast_recv(_c, type_id,data )\n", "  when 232\n      channel_id = _data.pop_nb_i4!()\n      type_id = _data.pop_nb_i4!()\n      data = _data.pop_nb_ia1!(65536)\n    $ssproto_channelcast_recv_counter += 1\n    ret = ssproto_channelcast_recv(_c, channel_id,type_id,data )\n", "  when 234\n      channel_id = _data.pop_nb_i4!()\n    $ssproto_join_channel_recv_counter += 1\n    ret = ssproto_join_channel_recv(_c, channel_id )\n", "  when 236\n    ret = ssproto_leave_channel_recv(_c)\n", "  when 240\n      x = _data.pop_nb_i4!()\n      y = _data.pop_nb_i4!()\n      range = _data.pop_nb_i4!()\n      type_id = _data.pop_nb_i4!()\n      data = _data.pop_nb_ia1!(65536)\n    $ssproto_nearcast_recv_counter += 1\n    ret = ssproto_nearcast_recv(_c, x,y,range,type_id,data )\n", "  when 242\n      x = _data.pop_nb_i4!()\n      y = _data.pop_nb_i4!()\n    $ssproto_update_nearcast_position_recv_counter += 1\n    ret = ssproto_update_nearcast_position_recv(_c, x,y )\n", "  when 244\n      channel_id = _data.pop_nb_i4!()\n    $ssproto_get_channel_member_count_recv_counter += 1\n    ret = ssproto_get_channel_member_count_recv(_c, channel_id )\n"]
  else
    ret = ssproto_sv_recv_error_callback(_c, "gencommand" )
  end
  ret
}
["def ssproto_pong_send( _c, t_usec, cmd )\n  _work = \"\"\n  _work.push_nb_i2!(2)\n  _work.push_nb_i8!( t_usec.to_i )\n  _work.push_nb_i4!( cmd.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_version_notify_send( _c, maj, min )\n  _work = \"\"\n  _work.push_nb_i2!(4)\n  _work.push_nb_i4!( maj.to_i )\n  _work.push_nb_i4!( min.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_conn_serial_result_send( _c, serial )\n  _work = \"\"\n  _work.push_nb_i2!(11)\n  _work.push_nb_i4!( serial.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_clean_all_result_send( _c )\n  _work = \"\"\n  _work.push_nb_i2!(13)\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_put_file_result_send( _c, query_id, result, filename )\n  _work = \"\"\n  _work.push_nb_i2!(21)\n  _work.push_nb_i4!( query_id.to_i )\n  _work.push_nb_i4!( result.to_i )\n  _work.push_nb_stra!( filename )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_get_file_result_send( _c, query_id, result, filename, data )\n  _work = \"\"\n  _work.push_nb_i2!(31)\n  _work.push_nb_i4!( query_id.to_i )\n  _work.push_nb_i4!( result.to_i )\n  _work.push_nb_stra!( filename )\n  _work.push_nb_ia1!( data.to_s )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_check_file_result_send( _c, query_id, result, filename )\n  _work = \"\"\n  _work.push_nb_i2!(41)\n  _work.push_nb_i4!( query_id.to_i )\n  _work.push_nb_i4!( result.to_i )\n  _work.push_nb_stra!( filename )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_ensure_image_result_send( _c, query_id, result, image_id )\n  _work = \"\"\n  _work.push_nb_i2!(51)\n  _work.push_nb_i4!( query_id.to_i )\n  _work.push_nb_i4!( result.to_i )\n  _work.push_nb_i4!( image_id.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_update_image_part_result_send( _c, query_id, result, image_id )\n  _work = \"\"\n  _work.push_nb_i2!(53)\n  _work.push_nb_i4!( query_id.to_i )\n  _work.push_nb_i4!( result.to_i )\n  _work.push_nb_i4!( image_id.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_get_image_png_result_send( _c, query_id, result, image_id, png_data )\n  _work = \"\"\n  _work.push_nb_i2!(55)\n  _work.push_nb_i4!( query_id.to_i )\n  _work.push_nb_i4!( result.to_i )\n  _work.push_nb_i4!( image_id.to_i )\n  _work.push_nb_ia1!( png_data.to_s )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_get_image_raw_result_send( _c, query_id, result, image_id, x0, y0, w, h, raw_data )\n  _work = \"\"\n  _work.push_nb_i2!(57)\n  _work.push_nb_i4!( query_id.to_i )\n  _work.push_nb_i4!( result.to_i )\n  _work.push_nb_i4!( image_id.to_i )\n  _work.push_nb_i4!( x0.to_i )\n  _work.push_nb_i4!( y0.to_i )\n  _work.push_nb_i4!( w.to_i )\n  _work.push_nb_i4!( h.to_i )\n  _work.push_nb_ia1!( raw_data.to_s )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_generate_id_32_result_send( _c, query_id, generated_id_start, num )\n  _work = \"\"\n  _work.push_nb_i2!(101)\n  _work.push_nb_i4!( query_id.to_i )\n  _work.push_nb_i4!( generated_id_start.to_i )\n  _work.push_nb_i4!( num.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_kvs_command_str_result_send( _c, query_id, retcode, valtype, result )\n  _work = \"\"\n  _work.push_nb_i2!(121)\n  _work.push_nb_i4!( query_id.to_i )\n  _work.push_nb_i4!( retcode.to_i )\n  _work.push_nb_i4!( valtype.to_i )\n  _work.push_nb_stra!( result )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_kvs_push_to_list_result_send( _c, query_id, retcode, key, updated_num )\n  _work = \"\"\n  _work.push_nb_i2!(131)\n  _work.push_nb_i4!( query_id.to_i )\n  _work.push_nb_i4!( retcode.to_i )\n  _work.push_nb_stra!( key )\n  _work.push_nb_i4!( updated_num.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_kvs_get_list_range_result_send( _c, query_id, retcode, start_ind, end_ind, key, result )\n  _work = \"\"\n  _work.push_nb_i2!(133)\n  _work.push_nb_i4!( query_id.to_i )\n  _work.push_nb_i4!( retcode.to_i )\n  _work.push_nb_i4!( start_ind.to_i )\n  _work.push_nb_i4!( end_ind.to_i )\n  _work.push_nb_stra!( key )\n  _work.push_nb_stra!( result )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_kvs_append_string_array_result_send( _c, query_id, retcode, key, field )\n  _work = \"\"\n  _work.push_nb_i2!(135)\n  _work.push_nb_i4!( query_id.to_i )\n  _work.push_nb_i4!( retcode.to_i )\n  _work.push_nb_stra!( key )\n  _work.push_nb_stra!( field )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_kvs_get_string_array_result_send( _c, query_id, retcode, key, field, result )\n  _work = \"\"\n  _work.push_nb_i2!(137)\n  _work.push_nb_i4!( query_id.to_i )\n  _work.push_nb_i4!( retcode.to_i )\n  _work.push_nb_stra!( key )\n  _work.push_nb_stra!( field )\n  _work.push_nb_stra!( result )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_kvs_save_bin_result_send( _c, query_id, retcode, valtype, key, field )\n  _work = \"\"\n  _work.push_nb_i2!(141)\n  _work.push_nb_i4!( query_id.to_i )\n  _work.push_nb_i4!( retcode.to_i )\n  _work.push_nb_i4!( valtype.to_i )\n  _work.push_nb_stra!( key )\n  _work.push_nb_stra!( field )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_kvs_load_bin_result_send( _c, query_id, retcode, has_data, key, field, data )\n  _work = \"\"\n  _work.push_nb_i2!(151)\n  _work.push_nb_i4!( query_id.to_i )\n  _work.push_nb_i4!( retcode.to_i )\n  _work.push_nb_i4!( has_data.to_i )\n  _work.push_nb_stra!( key )\n  _work.push_nb_stra!( field )\n  _work.push_nb_ia1!( data.to_s )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_counter_get_result_send( _c, counter_category, counter_id, result, curvalue )\n  _work = \"\"\n  _work.push_nb_i2!(162)\n  _work.push_nb_i4!( counter_category.to_i )\n  _work.push_nb_i4!( counter_id.to_i )\n  _work.push_nb_i4!( result.to_i )\n  _work.push_nb_i4!( curvalue.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_share_project_result_send( _c, project_id )\n  _work = \"\"\n  _work.push_nb_i2!(181)\n  _work.push_nb_i4!( project_id.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_publish_project_result_send( _c, project_id )\n  _work = \"\"\n  _work.push_nb_i2!(183)\n  _work.push_nb_i4!( project_id.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_search_shared_projects_result_send( _c, user_id, project_ids )\n  _work = \"\"\n  _work.push_nb_i2!(187)\n  _work.push_nb_i4!( user_id.to_i )\n  _work.push_nb_ia4!( project_ids )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_search_published_projects_result_send( _c, project_ids )\n  _work = \"\"\n  _work.push_nb_i2!(189)\n  _work.push_nb_ia4!( project_ids )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_project_is_joinable_result_send( _c, project_id, user_id, result )\n  _work = \"\"\n  _work.push_nb_i2!(191)\n  _work.push_nb_i4!( project_id.to_i )\n  _work.push_nb_i4!( user_id.to_i )\n  _work.push_nb_i4!( result.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_is_published_project_result_send( _c, project_id, published )\n  _work = \"\"\n  _work.push_nb_i2!(197)\n  _work.push_nb_i4!( project_id.to_i )\n  _work.push_nb_i4!( published.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_is_shared_project_result_send( _c, project_id, shared )\n  _work = \"\"\n  _work.push_nb_i2!(199)\n  _work.push_nb_i4!( project_id.to_i )\n  _work.push_nb_i4!( shared.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_list_presence_result_send( _c, project_id, user_ids )\n  _work = \"\"\n  _work.push_nb_i2!(205)\n  _work.push_nb_i4!( project_id.to_i )\n  _work.push_nb_ia4!( user_ids )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_count_presence_result_send( _c, project_id, user_num )\n  _work = \"\"\n  _work.push_nb_i2!(207)\n  _work.push_nb_i4!( project_id.to_i )\n  _work.push_nb_i4!( user_num.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_lock_grid_result_send( _c, grid_id, x, y, retcode )\n  _work = \"\"\n  _work.push_nb_i2!(211)\n  _work.push_nb_i4!( grid_id.to_i )\n  _work.push_nb_i4!( x.to_i )\n  _work.push_nb_i4!( y.to_i )\n  _work.push_nb_i4!( retcode.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_unlock_grid_result_send( _c, grid_id, x, y, retcode )\n  _work = \"\"\n  _work.push_nb_i2!(213)\n  _work.push_nb_i4!( grid_id.to_i )\n  _work.push_nb_i4!( x.to_i )\n  _work.push_nb_i4!( y.to_i )\n  _work.push_nb_i4!( retcode.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_lock_project_result_send( _c, project_id, category, retcode )\n  _work = \"\"\n  _work.push_nb_i2!(217)\n  _work.push_nb_i4!( project_id.to_i )\n  _work.push_nb_i4!( category.to_i )\n  _work.push_nb_i4!( retcode.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_unlock_project_result_send( _c, project_id, category, retcode )\n  _work = \"\"\n  _work.push_nb_i2!(219)\n  _work.push_nb_i4!( project_id.to_i )\n  _work.push_nb_i4!( category.to_i )\n  _work.push_nb_i4!( retcode.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_broadcast_notify_send( _c, type_id, sender_cli_id, data )\n  _work = \"\"\n  _work.push_nb_i2!(231)\n  _work.push_nb_i4!( type_id.to_i )\n  _work.push_nb_i4!( sender_cli_id.to_i )\n  _work.push_nb_ia1!( data.to_s )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_channelcast_notify_send( _c, channel_id, sender_cli_id, type_id, data )\n  _work = \"\"\n  _work.push_nb_i2!(233)\n  _work.push_nb_i4!( channel_id.to_i )\n  _work.push_nb_i4!( sender_cli_id.to_i )\n  _work.push_nb_i4!( type_id.to_i )\n  _work.push_nb_ia1!( data.to_s )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_join_channel_result_send( _c, channel_id, retcode )\n  _work = \"\"\n  _work.push_nb_i2!(235)\n  _work.push_nb_i4!( channel_id.to_i )\n  _work.push_nb_i4!( retcode.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_leave_channel_result_send( _c, retcode )\n  _work = \"\"\n  _work.push_nb_i2!(237)\n  _work.push_nb_i4!( retcode.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_nearcast_notify_send( _c, channel_id, sender_cli_id, x, y, range, type_id, data )\n  _work = \"\"\n  _work.push_nb_i2!(241)\n  _work.push_nb_i4!( channel_id.to_i )\n  _work.push_nb_i4!( sender_cli_id.to_i )\n  _work.push_nb_i4!( x.to_i )\n  _work.push_nb_i4!( y.to_i )\n  _work.push_nb_i4!( range.to_i )\n  _work.push_nb_i4!( type_id.to_i )\n  _work.push_nb_ia1!( data.to_s )\n  return ssproto_sv_sender(_c, _work )\nend\n", "def ssproto_get_channel_member_count_result_send( _c, channel_id, maxnum, curnum )\n  _work = \"\"\n  _work.push_nb_i2!(245)\n  _work.push_nb_i4!( channel_id.to_i )\n  _work.push_nb_i4!( maxnum.to_i )\n  _work.push_nb_i4!( curnum.to_i )\n  return ssproto_sv_sender(_c, _work )\nend\n"]
["def ssproto_get_ping_recv_count()\n  return $ssproto_ping_recv_counter\nend\n", "def ssproto_get_conn_serial_recv_count()\n  return $ssproto_conn_serial_recv_counter\nend\n", "def ssproto_get_clean_all_recv_count()\n  return $ssproto_clean_all_recv_counter\nend\n", "def ssproto_get_put_file_recv_count()\n  return $ssproto_put_file_recv_counter\nend\n", "def ssproto_get_get_file_recv_count()\n  return $ssproto_get_file_recv_counter\nend\n", "def ssproto_get_check_file_recv_count()\n  return $ssproto_check_file_recv_counter\nend\n", "def ssproto_get_ensure_image_recv_count()\n  return $ssproto_ensure_image_recv_counter\nend\n", "def ssproto_get_update_image_part_recv_count()\n  return $ssproto_update_image_part_recv_counter\nend\n", "def ssproto_get_get_image_png_recv_count()\n  return $ssproto_get_image_png_recv_counter\nend\n", "def ssproto_get_get_image_raw_recv_count()\n  return $ssproto_get_image_raw_recv_counter\nend\n", "def ssproto_get_generate_id_32_recv_count()\n  return $ssproto_generate_id_32_recv_counter\nend\n", "def ssproto_get_kvs_command_str_recv_count()\n  return $ssproto_kvs_command_str_recv_counter\nend\n", "def ssproto_get_kvs_command_str_oneway_recv_count()\n  return $ssproto_kvs_command_str_oneway_recv_counter\nend\n", "def ssproto_get_kvs_push_to_list_recv_count()\n  return $ssproto_kvs_push_to_list_recv_counter\nend\n", "def ssproto_get_kvs_get_list_range_recv_count()\n  return $ssproto_kvs_get_list_range_recv_counter\nend\n", "def ssproto_get_kvs_append_string_array_recv_count()\n  return $ssproto_kvs_append_string_array_recv_counter\nend\n", "def ssproto_get_kvs_get_string_array_recv_count()\n  return $ssproto_kvs_get_string_array_recv_counter\nend\n", "def ssproto_get_kvs_save_bin_recv_count()\n  return $ssproto_kvs_save_bin_recv_counter\nend\n", "def ssproto_get_kvs_load_bin_recv_count()\n  return $ssproto_kvs_load_bin_recv_counter\nend\n", "def ssproto_get_counter_add_recv_count()\n  return $ssproto_counter_add_recv_counter\nend\n", "def ssproto_get_counter_get_recv_count()\n  return $ssproto_counter_get_recv_counter\nend\n", "def ssproto_get_share_project_recv_count()\n  return $ssproto_share_project_recv_counter\nend\n", "def ssproto_get_publish_project_recv_count()\n  return $ssproto_publish_project_recv_counter\nend\n", "def ssproto_get_search_shared_projects_recv_count()\n  return $ssproto_search_shared_projects_recv_counter\nend\n", "def ssproto_get_search_published_projects_recv_count()\n  return $ssproto_search_published_projects_recv_counter\nend\n", "def ssproto_get_project_is_joinable_recv_count()\n  return $ssproto_project_is_joinable_recv_counter\nend\n", "def ssproto_get_unshare_project_recv_count()\n  return $ssproto_unshare_project_recv_counter\nend\n", "def ssproto_get_unpublish_project_recv_count()\n  return $ssproto_unpublish_project_recv_counter\nend\n", "def ssproto_get_is_published_project_recv_count()\n  return $ssproto_is_published_project_recv_counter\nend\n", "def ssproto_get_is_shared_project_recv_count()\n  return $ssproto_is_shared_project_recv_counter\nend\n", "def ssproto_get_update_presence_recv_count()\n  return $ssproto_update_presence_recv_counter\nend\n", "def ssproto_get_delete_presence_recv_count()\n  return $ssproto_delete_presence_recv_counter\nend\n", "def ssproto_get_list_presence_recv_count()\n  return $ssproto_list_presence_recv_counter\nend\n", "def ssproto_get_count_presence_recv_count()\n  return $ssproto_count_presence_recv_counter\nend\n", "def ssproto_get_lock_grid_recv_count()\n  return $ssproto_lock_grid_recv_counter\nend\n", "def ssproto_get_unlock_grid_recv_count()\n  return $ssproto_unlock_grid_recv_counter\nend\n", "def ssproto_get_lock_keep_grid_recv_count()\n  return $ssproto_lock_keep_grid_recv_counter\nend\n", "def ssproto_get_lock_project_recv_count()\n  return $ssproto_lock_project_recv_counter\nend\n", "def ssproto_get_unlock_project_recv_count()\n  return $ssproto_unlock_project_recv_counter\nend\n", "def ssproto_get_lock_keep_project_recv_count()\n  return $ssproto_lock_keep_project_recv_counter\nend\n", "def ssproto_get_broadcast_recv_count()\n  return $ssproto_broadcast_recv_counter\nend\n", "def ssproto_get_channelcast_recv_count()\n  return $ssproto_channelcast_recv_counter\nend\n", "def ssproto_get_join_channel_recv_count()\n  return $ssproto_join_channel_recv_counter\nend\n", "def ssproto_get_leave_channel_recv_count()\n  return $ssproto_leave_channel_recv_counter\nend\n", "def ssproto_get_nearcast_recv_count()\n  return $ssproto_nearcast_recv_counter\nend\n", "def ssproto_get_update_nearcast_position_recv_count()\n  return $ssproto_update_nearcast_position_recv_counter\nend\n", "def ssproto_get_get_channel_member_count_recv_count()\n  return $ssproto_get_channel_member_count_recv_counter\nend\n", "def ssproto_get_pong_send_count()\n  return $ssproto_pong_send_counter\nend\n", "def ssproto_get_version_notify_send_count()\n  return $ssproto_version_notify_send_counter\nend\n", "def ssproto_get_conn_serial_result_send_count()\n  return $ssproto_conn_serial_result_send_counter\nend\n", "def ssproto_get_clean_all_result_send_count()\n  return $ssproto_clean_all_result_send_counter\nend\n", "def ssproto_get_put_file_result_send_count()\n  return $ssproto_put_file_result_send_counter\nend\n", "def ssproto_get_get_file_result_send_count()\n  return $ssproto_get_file_result_send_counter\nend\n", "def ssproto_get_check_file_result_send_count()\n  return $ssproto_check_file_result_send_counter\nend\n", "def ssproto_get_ensure_image_result_send_count()\n  return $ssproto_ensure_image_result_send_counter\nend\n", "def ssproto_get_update_image_part_result_send_count()\n  return $ssproto_update_image_part_result_send_counter\nend\n", "def ssproto_get_get_image_png_result_send_count()\n  return $ssproto_get_image_png_result_send_counter\nend\n", "def ssproto_get_get_image_raw_result_send_count()\n  return $ssproto_get_image_raw_result_send_counter\nend\n", "def ssproto_get_generate_id_32_result_send_count()\n  return $ssproto_generate_id_32_result_send_counter\nend\n", "def ssproto_get_kvs_command_str_result_send_count()\n  return $ssproto_kvs_command_str_result_send_counter\nend\n", "def ssproto_get_kvs_push_to_list_result_send_count()\n  return $ssproto_kvs_push_to_list_result_send_counter\nend\n", "def ssproto_get_kvs_get_list_range_result_send_count()\n  return $ssproto_kvs_get_list_range_result_send_counter\nend\n", "def ssproto_get_kvs_append_string_array_result_send_count()\n  return $ssproto_kvs_append_string_array_result_send_counter\nend\n", "def ssproto_get_kvs_get_string_array_result_send_count()\n  return $ssproto_kvs_get_string_array_result_send_counter\nend\n", "def ssproto_get_kvs_save_bin_result_send_count()\n  return $ssproto_kvs_save_bin_result_send_counter\nend\n", "def ssproto_get_kvs_load_bin_result_send_count()\n  return $ssproto_kvs_load_bin_result_send_counter\nend\n", "def ssproto_get_counter_get_result_send_count()\n  return $ssproto_counter_get_result_send_counter\nend\n", "def ssproto_get_share_project_result_send_count()\n  return $ssproto_share_project_result_send_counter\nend\n", "def ssproto_get_publish_project_result_send_count()\n  return $ssproto_publish_project_result_send_counter\nend\n", "def ssproto_get_search_shared_projects_result_send_count()\n  return $ssproto_search_shared_projects_result_send_counter\nend\n", "def ssproto_get_search_published_projects_result_send_count()\n  return $ssproto_search_published_projects_result_send_counter\nend\n", "def ssproto_get_project_is_joinable_result_send_count()\n  return $ssproto_project_is_joinable_result_send_counter\nend\n", "def ssproto_get_is_published_project_result_send_count()\n  return $ssproto_is_published_project_result_send_counter\nend\n", "def ssproto_get_is_shared_project_result_send_count()\n  return $ssproto_is_shared_project_result_send_counter\nend\n", "def ssproto_get_list_presence_result_send_count()\n  return $ssproto_list_presence_result_send_counter\nend\n", "def ssproto_get_count_presence_result_send_count()\n  return $ssproto_count_presence_result_send_counter\nend\n", "def ssproto_get_lock_grid_result_send_count()\n  return $ssproto_lock_grid_result_send_counter\nend\n", "def ssproto_get_unlock_grid_result_send_count()\n  return $ssproto_unlock_grid_result_send_counter\nend\n", "def ssproto_get_lock_project_result_send_count()\n  return $ssproto_lock_project_result_send_counter\nend\n", "def ssproto_get_unlock_project_result_send_count()\n  return $ssproto_unlock_project_result_send_counter\nend\n", "def ssproto_get_broadcast_notify_send_count()\n  return $ssproto_broadcast_notify_send_counter\nend\n", "def ssproto_get_channelcast_notify_send_count()\n  return $ssproto_channelcast_notify_send_counter\nend\n", "def ssproto_get_join_channel_result_send_count()\n  return $ssproto_join_channel_result_send_counter\nend\n", "def ssproto_get_leave_channel_result_send_count()\n  return $ssproto_leave_channel_result_send_counter\nend\n", "def ssproto_get_nearcast_notify_send_count()\n  return $ssproto_nearcast_notify_send_counter\nend\n", "def ssproto_get_get_channel_member_count_result_send_count()\n  return $ssproto_get_channel_member_count_result_send_counter\nend\n"]

def ssproto_sv_get_version()
  return [ 10003 , 42653699 ]
end

# End of generated code
