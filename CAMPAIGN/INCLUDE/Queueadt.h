// =========
// Queue ADT
// =========

// -----------------
// User Descriptions
// -----------------

 /*���������������������������������������������������������������������ͻ
   � QueueElement CreateQueueElement (void)                              �
   ���������������������������������������������������������������������Ķ
   � Creates and returns a Queue Element.                                �
   ���������������������������������������������������������������������Ķ
   � Author: Gary L. Stottlemyer      � Inquiries: Gary L. Stottlemyer   �
   � Project: Falcon 4.0              � Current Project: Falcon 4.0      �
   ���������������������������������������������������������������������ͼ*/

 /*���������������������������������������������������������������������ͻ
   � void DisposeQueueElement (QueueElement E)                           �
   ���������������������������������������������������������������������Ķ
   � Disposes a Queue Element.                                           �
   �                                                                     �
   � WARNING: Does NOT dispose of any User Data attached to the Element. �
   ���������������������������������������������������������������������Ķ
   � Author: Gary L. Stottlemyer      � Inquiries: Gary L. Stottlemyer   �
   � Project: Falcon 4.0              � Current Project: Falcon 4.0      �
   ���������������������������������������������������������������������ͼ*/

 /*���������������������������������������������������������������������ͻ
   � void SetQueueElementUserData (QueueElement E, void* UserDataPointer)�
   ���������������������������������������������������������������������Ķ
   � Allows User Defined Data to be attached to a Queue Element.         �
   ���������������������������������������������������������������������Ķ
   � Author: Gary L. Stottlemyer      � Inquiries: Gary L. Stottlemyer   �
   � Project: Falcon 4.0              � Current Project: Falcon 4.0      �
   ���������������������������������������������������������������������ͼ*/

 /*���������������������������������������������������������������������ͻ
   � void* GetQueueElementUserData (QueueElement E)                      �
   ���������������������������������������������������������������������Ķ
   � Allows access to any User Defined Data previously attached to the   �
   � given Queue Element.                                                �
   ���������������������������������������������������������������������Ķ
   � Author: Gary L. Stottlemyer      � Inquiries: Gary L. Stottlemyer   �
   � Project: Falcon 4.0              � Current Project: Falcon 4.0      �
   ���������������������������������������������������������������������ͼ*/

 /*���������������������������������������������������������������������ͻ
   � Queue CreateQueue (void)                                            �
   ���������������������������������������������������������������������Ķ
   � Creates an "empty" Queue.                                           �
   ���������������������������������������������������������������������Ķ
   � Author: Gary L. Stottlemyer      � Inquiries: Gary L. Stottlemyer   �
   � Project: Falcon 4.0              � Current Project: Falcon 4.0      �
   ���������������������������������������������������������������������ͼ*/

 /*���������������������������������������������������������������������ͻ
   � void EnqueueElement (Queue Q, QueueElement E)                       �
   ���������������������������������������������������������������������Ķ
   � Enqueues a Queue Element onto a Queue.                              �
   ���������������������������������������������������������������������Ķ
   � Author: Gary L. Stottlemyer      � Inquiries: Gary L. Stottlemyer   �
   � Project: Falcon 4.0              � Current Project: Falcon 4.0      �
   ���������������������������������������������������������������������ͼ*/

 /*���������������������������������������������������������������������ͻ
   � QueueElement DequeueElement (Queue Q)ement E)                       �
   ���������������������������������������������������������������������Ķ
   � Dequeues and returns a Queue Element from a Queue.                  �
   ���������������������������������������������������������������������Ķ
   � Author: Gary L. Stottlemyer      � Inquiries: Gary L. Stottlemyer   �
   � Project: Falcon 4.0              � Current Project: Falcon 4.0      �
   ���������������������������������������������������������������������ͼ*/

 /*���������������������������������������������������������������������ͻ
   � void DisposeQueue (Queue Q)                                         �
   ���������������������������������������������������������������������Ķ
   � Disposes a Queue and all Queue Elements contained within that Queue.�
   �                                                                     �
   � WARNING: Does NOT dispose of any User Data attached to the Elements �
   �          contained in the Queue.                                    �
   ���������������������������������������������������������������������Ķ
   � Author: Gary L. Stottlemyer      � Inquiries: Gary L. Stottlemyer   �
   � Project: Falcon 4.0              � Current Project: Falcon 4.0      �
   ���������������������������������������������������������������������ͼ*/

// ---------------------------------------
// Type and External Function Declarations
// ---------------------------------------

   typedef void* QueueElement;
   typedef void* Queue;

   extern QueueElement CreateQueueElement (void);

   extern void DisposeQueueElement (QueueElement E);

   extern void SetQueueElementUserData (QueueElement E, void* UserDataPointer);

   extern void* GetQueueElementUserData (QueueElement E);

   extern Queue CreateQueue (void);

   extern void EnqueueElement (Queue Q, QueueElement E);

   extern QueueElement DequeueElement (Queue Q);

   extern void DisposeQueue (Queue Q);
