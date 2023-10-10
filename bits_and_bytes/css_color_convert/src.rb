HEX_VALUE_REGEX = /#([a-fA-F0-9]{2})([a-fA-F0-9]{2})([a-fA-F0-9]{2})/

def main(file_path = ARGV[0])
  f = File.open(file_path, 'r+')
  str = f.read

  replacement = str.gsub(HEX_VALUE_REGEX) do |match|
    r_val = hex_to_int_the_hard_way($1)
    g_val = hex_to_int_the_hard_way($2)
    b_val = hex_to_int_the_hard_way($3)

    "rgb(#{r_val} #{g_val} #{b_val})"
  end

  f.truncate(0)
  f.rewind
  f.write(replacement)
  f.close
end

def hex_to_int_the_easy_way(hex_string)
  hex_string.to_i(16)
end

def hex_to_int_the_hard_way(hex_string)
  hex_string
    .reverse                               # work from LSB to MSB (so that index is the correct exponent)
    .split("")
    .map
    .with_index do |digit_str, exp|
      ascii_value = digit_str.ord
      place_value = 
        case ascii_value
        when ("A".ord.."F".ord)
          ascii_value - 55                 # A's ASCII code is 65 - 55 = 10 (hex value)
        when ("a".ord.."f".ord)
          ascii_value - 87                 # a's ASCII code is 97 - 87 = 10 (hex value)
        when ("0".ord.."9".ord)
          ascii_value - 48                 # 0's ASCII code is 48 - 48 = 0 (hex value)
        else
          raise "unknown hex value: #{digit_str}"
        end

        place_value * (16 ** exp)
    end.sum
end

main