# Ruby version
class Book
  attr_accessor :title, :author, :copies

  def initialize(title, author, copies)
    @title = title
    @author = author
    @copies = copies
  end
end

def save_book(book)
  File.open("library.txt", "a") do |file|
    file.puts "#{book.title} | #{book.author} | #{book.copies} copies"
  end
rescue IOError
end

def show_books
  unless File.exist?("library.txt")
    puts "No records found."
    return
  end

  puts "\n--- Library Records ---"
  File.foreach("library.txt") { |line| print line }
end

print "Enter title: "
title = gets.chomp
print "Enter author: "
author = gets.chomp
print "Enter copies: "
copies = gets.to_i

save_book(Book.new(title, author, copies))
show_books
