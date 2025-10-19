---
layout: default
title: "Artifact 1 – CS-405 Secure Coding: Encryption Enhancement"
---

[Home](index.md) | [Code Review](code-review.md) | [CS-405](artifact1-cs405.md) | [CS-300](artifact2-cs300.md) | [CS-340](artifact3-cs340.md)

---

# Artifact 1 – CS-405 Secure Coding: Encryption Enhancement

### Overview
This enhancement is from **CS-405: Secure Coding** and focuses on improving a C++ encryption program by applying industry security standards and software design best practices.  
The goal of this enhancement was to modernize and improve the logic, improve maintainability, and implement secure handling consistent with SEI CERT C++ guidelines.

---

### Original Artifact
The original program (`Enhancement1_ORIGINAL_KM_M5_Excryption.cpp`) implemented a basic encryption/decryption function with minimal error handling and no secure key derivation.

➡️ [View Original Source Code](./Enhancement1_ORIGINAL_KM_M5_Excryption.cpp)

---

### Enhanced Artifact
The enhanced version (`Enhancement1_KM_M5_Encryption.cpp`) introduces:
- **Improved modular structure** and use of reusable functions  
- **Key derivation with PBKDF2** and stronger encryption algorithms  
- **Exception handling and input validation**  
- **In-code documentation and SEI CERT C++ compliance**

➡️ [View Enhanced Source Code](./Enhancement1_KM_M5_Encryption.cpp)  
🖼️ [View Verification Screenshot](./Verification1.jpg)  
📄 [Download Narrative (.docx)](./KM_Milestone2_Narrative.docx)

---

### Reflection
This enhancement demonstrates my ability to refactor insecure legacy code into more compliant and modern code. 
I applied secure software engineering practices, including password hashing, modular function design, and static code analysis to identify potential flaws.  

By redesigning the encryption process and improving key management, I strengthened the overall reliability and security of the system, aligning with industry standards.

This enhancement also showcases:
- **Software design & modular engineering**  
- **Secure coding principles**  
- **Error handling and exception safety**

---

### Course Outcomes Demonstrated
- **Design, develop, and deliver professional-quality code** adhering to secure software engineering standards.  
- **Develop a security mindset** by mitigating vulnerabilities and applying SEI CERT C++ best practices.  
- **Employ innovative tools and techniques** (such as PBKDF2 and static analysis) to produce secure, reliable software.
