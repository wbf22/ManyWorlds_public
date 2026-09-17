




# note
All the classes in here shouldn't manage any server resources. We're keeping all those in the Data class. This will help make sure all synchronization is managed in that class, and these classes can perform logic without worrying about synchronization.



These classes represent the endoints for different functions in the server. By default they're over UDP, except for auth requests such as login or register. Every request to these should come in with a access token so we can verify who is making the request.


