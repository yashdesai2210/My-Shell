pwd
#should print pwd

#echo crashingggg
echo hello
#should ignore comments and only print hello

cd subdir
pwd
cd ..
#should go into subdir and print path

cd dcthedonDir
#should error (non existent dir)

which echo
which fakecommand
which cd
#external command, DNE, and shell built in

ls
#should list all files

/bin/pwd
#should print dir even with pathname to pwd instead of simply command


echo this is a test > output.txt
cat output.txt

cat < output.txt

echo "input" > input.txt
sort < input.txt > sorted.txt
cat sorted.txt

pwd | cat
#redirects pwd back into terminal, tests pipeline

ls | CartiAintDroppin
#tests pipeline error

# Test Case 5.6 & 5.7: Pipelines with Redirection in Subcommands
# (These tests are based on the provided code behavior)
cat < input.txt | wc -l
ls | sort > sorted_output.txt
cat sorted_output.txt

pwd
and echo "print this"
#tests "and" with success


cd dirNotHere
and echo "this no print :("
#tests "and" with failure

asdasd | wc -l
and echo "print this"
#tests "and" with good pipe

wc -l | asdasda
and echo "dont print"
#tests "and" with failed pipe

cd dirNotHere
or echo "this print :)"
#tests "or" with success

pwd
or echo "this no print :("
#tests "or" with failure

pwd
and cd NoMaxxing
or echo "printMaxxing"
#should print since pwd succeeds but and fails

echo *.txt
#prints all *.txt files

echo "Files matching *file (should exclude hidden files):"
echo *file

exit
