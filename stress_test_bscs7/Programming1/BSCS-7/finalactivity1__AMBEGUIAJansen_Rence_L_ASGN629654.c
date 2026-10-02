# Python version
class Student:
    def __init__(self):
        self.name = ""
        self.age = 0
        self.gpa = 0.0

student = Student()
student.name = input("Enter name: ")
student.age = int(input("Enter age: "))
student.gpa = float(input("Enter GPA: "))

print(f"\nName: {student.name}")
print(f"Age: {student.age}")
print(f"GPA: {student.gpa:.2f}")
